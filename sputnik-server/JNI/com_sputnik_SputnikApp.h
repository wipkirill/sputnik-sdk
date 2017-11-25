#pragma once
#include <jni.h>
#include <string>
/* Header for class com_urbanlabs_sdk_Sputnik */

#ifndef _Included_com_urbanlabs_sdk_Sputnik
#define _Included_com_urbanlabs_sdk_Sputnik

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    initSputnikServer
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_urbanlabs_sdk_Sputnik_initSputnikServer
  (JNIEnv *, jclass, jint, jstring, jstring);

/*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    stopSputnikServer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_urbanlabs_sdk_Sputnik_stopSputnikServer
  (JNIEnv *, jclass);

/*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    sputnikServerReady
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_urbanlabs_sdk_Sputnik_sputnikServerReady
  (JNIEnv *, jclass);

/*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    getTileNative
 * Signature: ()Z
 */
JNIEXPORT void JNICALL Java_com_urbanlabs_sdk_Sputnik_getTileBitmap
  (JNIEnv *, jclass, jobject bitmap, jstring jMapName, jstring jLayerId, jint x, jint y, jint z);

  /*
 * Class:     com_urbanlabs_sdk_Sputnik
 * Method:    getTileNative
 * Signature: ()Z
 */
JNIEXPORT jstring JNICALL Java_com_urbanlabs_sdk_Sputnik_getTileString
  (JNIEnv *, jclass, jstring jMapName, jstring jLayerId, jint x, jint y, jint z, jstring ext);

#ifdef __cplusplus
}
#endif

#endif
