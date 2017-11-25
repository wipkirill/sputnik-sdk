#ifdef ANDROID
#include <thread>
#include <memory>
#include <exception>

#include <boost/optional.hpp>

#include <android/bitmap.h>

#include "JNI/com_sputnik_SputnikApp.h"
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <WebService/Main.h>

using namespace std;

namespace {

// cache this for future calls
static jclass BITMAP_CLASS;
static jmethodID BITMAP_SETPIXELS;

struct JniStrDeleter {
    JniStrDeleter(JNIEnv* env, jstring& jstr) : env_(env), jstr_(jstr) {}

    void operator() (const char* str) {
        if (str) {
            env_->ReleaseStringUTFChars(jstr_, str);
        }
    }
private:
    JNIEnv* env_;
    jstring& jstr_;
};

typedef std::unique_ptr<const char, JniStrDeleter> JniStr;

std::string toCppString(JNIEnv* env, jstring& jstr) {
    JniStr str(env->GetStringUTFChars(jstr, 0), {env, jstr});
    if (str.get() != nullptr) {
        return std::string(str.get());
    } else {
        throw std::runtime_error("jstr set to null value in function toCppString.");
    }
}

// NOTE: ActivityThread.currentPackageName() only returns non-null if the
// current thread is an application main thread.
// returns null otherwise.
// Null reproduced on 4.1.2, works on Android 5, 4.4.2
boost::optional<std::string> jniGetPackageName(JNIEnv *env) {
    jclass classRef = env->FindClass("android/app/ActivityThread");
    if(classRef == nullptr) {
        LOGG(Logger::ERROR) << "Class ActivityThread not found." << Logger::FLUSH;
        return boost::none;
    } else {
        LOGG(Logger::INFO) << "Class ActivityThread found." << Logger::FLUSH;
    }

    jmethodID staticMethod = env->GetStaticMethodID(classRef,
        "currentPackageName", "()Ljava/lang/String;");
    if(staticMethod == nullptr) {
        LOGG(Logger::ERROR) << "Method currentPackageName not found." << Logger::FLUSH;
        return boost::none;
    } else {
        LOGG(Logger::INFO) << "Method currentPackageName found." << Logger::FLUSH;
    }

    jstring result = (jstring)env->CallStaticObjectMethod(classRef, staticMethod);
    LOGG(Logger::INFO) << "Called method currentPackageName." << Logger::FLUSH;
    return toCppString(env, result);
}

void runServer(int port, const std::string& workDir, const std::string& licenseFilePath, const std::string& packageName) {
    try {
        GeoRouting app(port, workDir);
        GeoRouting::instance_ = &app;
        if(app.loadLicense(licenseFilePath, packageName)) {
            app.Run();
            app.waitForTermination();
        }
    } catch(exception& e) {
        LOGG(Logger::INFO) << "Caught exception" << e.what() << Logger::FLUSH;
    }
}
/*
* Return a*b/255, rounding any fractional bits. Only valid if both
* a and b are 0..255
*/
static inline uint8_t SkMulDiv255Round(uint8_t a, uint8_t b) {
   unsigned prod = a*1u*b + 128;
   return (prod + (prod >> 8)) >> 8;
}
/*
* Detect endianess
*/
static inline bool littleEndian() {
    unsigned short word = 0x0102;
    return *(char*)&word == 2;
}

} // anonymous namespace

// According to http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/invocation.html#JNI_OnLoad
// The VM calls JNI_OnLoad when the native library is loaded (for example, through System.loadLibrary).
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    // Get JNI Env for all function calls
    JNIEnv* env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    BITMAP_CLASS = env->FindClass("android/graphics/Bitmap");
    if (BITMAP_CLASS == nullptr) {
        LOGG(Logger::ERROR) << "No class Bitmap found" << Logger::FLUSH;
        return JNI_ERR;
    }

    BITMAP_SETPIXELS = env->GetMethodID(BITMAP_CLASS, "setPixels", "([IIIIIII)V");
    if(BITMAP_SETPIXELS == nullptr) {
        LOGG(Logger::ERROR) << "No method Bitmap.setPixels found" << Logger::FLUSH;
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

/*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    initSputnikServer
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_urbanlabs_sdk_Sputnik_initSputnikServer
  (JNIEnv *env, jclass obj, jint jPort, jstring jWorkDir, jstring jLicenseFile) {
    INIT_LOGGING(Logger::INFO);
    try {
        boost::optional<std::string> packageName = jniGetPackageName(env);
        if (!packageName) {
            LOGG(Logger::ERROR) << "Failed to get packageName" << Logger::FLUSH;
            return;
        } else {
            LOGG(Logger::INFO) << "Package name found: " << packageName.get() << Logger::FLUSH;
        }

        std::thread serverThread(runServer, (int)jPort, toCppString(env, jWorkDir),
            toCppString(env, jLicenseFile), packageName.get());
        serverThread.detach();
    } catch(exception& e) {
        LOGG(Logger::INFO) << "Caught exception" << e.what() << Logger::FLUSH;
    }
}

/*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    getTileNative
 * Returns bitmap object filled in by Mapnik image data
 */
JNIEXPORT void JNICALL Java_com_urbanlabs_sdk_Sputnik_getTileBitmap
  (JNIEnv *env, jclass obj, jobject bitmap, jstring jMapName, jstring jLayerId, jint x, jint y, jint z) {
    mapnik::image_32 tile =
        GeoRouting::instance_->getTileBitmap(toCppString(env, jMapName),
            toCppString(env, jLayerId), (int)x, (int)y, (int)z);

    AndroidBitmapInfo bitmapInfo;
    if (AndroidBitmap_getInfo(env, bitmap, &bitmapInfo) < 0) {
        LOGG(Logger::ERROR) << "AndroidBitmap_getInfo() failed." << Logger::FLUSH;
        return;
    }

    if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGG(Logger::ERROR) << "Bitmap format is not RGBA_8888." << Logger::FLUSH;
        return;
    }

    // stride is larger or equal to the size of a row in bytes
    int width = tile.width(), height = tile.height();
    if (bitmapInfo.stride != width*4) {
        LOGG(Logger::ERROR) << "Bitmap stride is larger than width." << Logger::FLUSH;
        return;
    }

    if (bitmapInfo.width != width || bitmapInfo.height != height) {
        LOGG(Logger::ERROR) << "Image width or height different from tile size." << Logger::FLUSH;
        return;
    }

    // TODO this is how to lock pixels in place and update values.
    // it looses alpha channel and breaks colors, since some modification on color has to
    // be done be before writing to the array
    void* bitmapPixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels) < 0) {
        LOGG(Logger::ERROR) << "AndroidBitmap_lockPixels() failed." << Logger::FLUSH;
        return;
    }

    bool lEndian = littleEndian();

    uint32_t* newBitmapPixels = (uint32_t*) bitmapPixels;
    for(int row = 0; row < height; ++row) {
        const unsigned* imageRow = tile.get_row(row);
        for (int i = 0 ; i < width; ++i) {
            if(lEndian) {
                unsigned char alpha = (imageRow[i] >> 24) & 0xff;
                unsigned char red = (imageRow[i] >> 16) & 0xff;
                unsigned char green = (imageRow[i] >> 8) & 0xff;
                unsigned char blue = (imageRow[i]) & 0xff;
                uint32_t currentPixel = (alpha << 24) | ( SkMulDiv255Round(red, alpha) << 16) | (SkMulDiv255Round(green, alpha) << 8) | (SkMulDiv255Round(blue, alpha));
                newBitmapPixels[row * width + i] = currentPixel;
            } else {
                unsigned char alpha = (imageRow[i]) & 0xff;
                unsigned char red = (imageRow[i] >> 8) & 0xff;
                unsigned char green = (imageRow[i] >> 16) & 0xff;
                unsigned char blue = (imageRow[i] >> 24) & 0xff;
                uint32_t currentPixel = (SkMulDiv255Round(blue, alpha) << 24) | ( SkMulDiv255Round(green, alpha) << 16) | (SkMulDiv255Round(red, alpha) << 8) | (alpha);
                newBitmapPixels[row * width + i] = currentPixel;
            }
        }
    }

    AndroidBitmap_unlockPixels(env, bitmap);

    // This allocates a new array and calls setPixels on the bitmap.
    // Potentially is safer than previous code, since avoids hacks
    // Discussion: Android API might change but Bitmap.setPixels is guaranteed to do right job
    // int width = tile.width(), height = tile.height();
    // jintArray pixels = env->NewIntArray(width * height);
    // for(int row = 0; row < height; ++row) {
    //     const unsigned* imageRow = tile.get_row(row);
    //     for (int i = 0; i < width; ++i) {
    //         unsigned char red   = (imageRow[i]) & 0xff;
    //         unsigned char green = (imageRow[i] >> 8) & 0xff;
    //         unsigned char blue  = (imageRow[i] >> 16) & 0xff;
    //         unsigned char alpha = (imageRow[i] >> 24) & 0xff;
    //         int currentPixel = (alpha << 24) | (red << 16) | (green << 8) | (blue);
    //         env->SetIntArrayRegion(pixels, row * width + i, 1, &currentPixel);
    //     }
    // }
    // env->CallVoidMethod(bitmap, BITMAP_SETPIXELS, pixels, 0, width, 0, 0, width, height);
}

JNIEXPORT jstring JNICALL Java_com_urbanlabs_sdk_Sputnik_getTileString
  (JNIEnv* env, jclass obj, jstring jMapName, jstring jLayerId, jint x, jint y, jint z, jstring jExt) {
    string mapName = toCppString(env, jMapName);
    string layerId = toCppString(env, jLayerId);
    string ext = toCppString(env, jExt);

    string tile = GeoRouting::instance_->getTileString(mapName, layerId, (int)x, (int)y, (int)z, ext);
    return env->NewStringUTF(tile.c_str());
}

/*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    stopSputnikServer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_urbanlabs_sdk_Sputnik_stopSputnikServer
  (JNIEnv *, jclass) {
    ;
}

/*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    ready
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_urbanlabs_sdk_Sputnik_sputnikServerReady
  (JNIEnv *, jclass) {
    return GeoRouting::isStarted();
}

#endif
