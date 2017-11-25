#include <UrbanLabs/Sdk/Concurrent/LongTask.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/Compression.h>

using namespace std;

//#ifndef ANDROID
///* this is how the CURLOPT_XFERINFOFUNCTION callback works */
//int DownloadTask::xferinfo(void* p,
                           //curl_off_t dltotal, curl_off_t dlnow,
                           //curl_off_t /*ultotal*/, curl_off_t /*ulnow*/)
//{
    //TaskStatus* myp = (TaskStatus*)p;
    //if (dltotal > 0) {
        //myp->setPercentage(100.0 * (double)dlnow / (double)dltotal);
    //}
    //return 0;
//}
//
///* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */
//int DownloadTask::older_progress(void* p,
                                 //double dltotal, double dlnow,
                                 //double ultotal, double ulnow)
//{
    //return xferinfo(p,
                    //(curl_off_t)dltotal,
                    //(curl_off_t)dlnow,
                    //(curl_off_t)ultotal,
                    //(curl_off_t)ulnow);
//}
///**
 //* @brief write_data
 //* @param ptr
 //* @param size
 //* @param nmemb
 //* @param stream
 //* @return
 //*/
//size_t DownloadTask::write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
//{
    //return fwrite(ptr, size, nmemb, stream);
//}
///**
 //* @brief run
 //* @param downloadUrl
 //* @param props
 //* @param progress
 //*/
//void DownloadTask::run(TaskStatus& progress, const URL& downloadUrl, const URL& outputUrl)
//{
    //if (CURL* curl = curl_easy_init()) {
        //progress.setState(TaskStatus::RUNNING);
        //FILE* fp = fopen(outputUrl.getPath().c_str(), "wb");
//
        //curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.getPath().c_str());
        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
//
        //curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, older_progress);
        ///* pass the struct pointer into the progress function */
        //curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress);
//
//#if LIBCURL_VERSION_NUM >= 0x072000
        ///* xferinfo was introduced in 7.32.0, no earlier libcurl versions will
           //compile as they won't have the symbols around.
//
           //If built with a newer libcurl, but running with an older libcurl:
           //curl_easy_setopt() will fail in run-time trying to set the new
           //callback, making the older callback get used.
//
           //New libcurls will prefer the new callback and instead use that one even
           //if both callbacks are set. */
//
        //curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        ///* pass the struct pointer into the xferinfo function, note that this is
           //an alias to CURLOPT_PROGRESSDATA */
        //curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
//#endif
//
        //curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        //CURLcode res = curl_easy_perform(curl);
//
        //if (res != CURLE_OK) {
            //string err = string(curl_easy_strerror(res));
            //LOGG(Logger::ERROR) << "Download failed:" << err << Logger::FLUSH;
            //progress.setState(TaskStatus::FAILED);
            //progress.setErrorMessage(err);
            //fclose(fp);
            //std::remove(outputUrl.getPath().c_str());
            //return;
        //}
//
        ///* always cleanup */
        //curl_easy_cleanup(curl);
        //fclose(fp);
        //progress.setState(TaskStatus::SUCCESS);
    //} else {
        //progress.setState(TaskStatus::FAILED);
        //string err = "Failed to initialize download";
        //LOGG(Logger::ERROR) << err << Logger::FLUSH;
        //progress.setErrorMessage(err);
    //}
//}
//#endif
/**
 * @brief DecompressTask::run
 * @param progress
 * @param inputUrl
 * @param outputUrl
 */
void DecompressTask::run(TaskStatus& progress, const URL& inputUrl, const URL& outputUrl)
{
    progress.setState(TaskStatus::RUNNING);
    if (BzipUtil::isDecompressable(inputUrl.getPath())) {
        if (!BzipUtil::decompress(inputUrl.getPath(), outputUrl.getPath(), true)) {
            progress.setState(TaskStatus::FAILED);
            return;
        }
    } else {
        LOGG(Logger::WARNING) << "[DECOMPRESS] Nothing to decompress, file is not an archive"
                              << inputUrl.getPath() << Logger::FLUSH;
    }

    progress.setPercentage(100.0);
    progress.setState(TaskStatus::SUCCESS);
}
