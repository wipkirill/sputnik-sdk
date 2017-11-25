#include <UrbanLabs/Sdk/Utils/Compression.h>
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

using namespace std;

bool BzipUtil::isDecompressable(const string &infileName) {
    //if(!FsUtil::fileExists(infileName)) {
        //LOGG(Logger::ERROR) << "[BZip] Input file does not exist" << Logger::FLUSH;
        //return false;
    //}
    //FILE * archiveDesc = NULL;
    //if((archiveDesc = fopen(infileName.c_str(),"rb"))==NULL){
        //LOGG(Logger::ERROR) << "[BZip] Cannot open input file " <<infileName << Logger::FLUSH;
        //return false;
    //}
    //int bzError;
    //BZFILE *bzf;
    //char buf[4096];
    //bool ret = true;
    //bzf = BZ2_bzReadOpen(&bzError, archiveDesc, 0, 0, NULL, 0);
    //if (bzError != BZ_OK) {
        //LOGG(Logger::ERROR) << "[BZip] Cannot initialize bzip on " <<infileName << Logger::FLUSH;
        //ret = false;
    //}
    //BZ2_bzRead(&bzError, bzf, buf, sizeof buf);
    //if (bzError != BZ_OK) {
        //ret = false;
    //}
    //BZ2_bzReadClose(&bzError, bzf);
    return true;
}


/**
 * @brief BzipUtil::decompress
 * @param infileName
 * @param outFileName
 * @return
 */
bool BzipUtil::decompress(const string &infileName, const string &outFileName, bool removeSrc) {
    //if(!FsUtil::fileExists(infileName)) {
        //LOGG(Logger::ERROR) << "[BZip] Input file does not exist" << Logger::FLUSH;
        //return false;
    //}
    //if(FsUtil::fileExists(outFileName)) {
        //LOGG(Logger::WARNING) << "[BZip] Overwriting output file "<<outFileName << Logger::FLUSH;
    //}
    //FILE * archiveDesc = NULL;
    //if((archiveDesc = fopen(infileName.c_str(),"rb"))==NULL){
        //LOGG(Logger::ERROR) << "[BZip] Cannot open input file " <<infileName << Logger::FLUSH;
        //return false;
    //}
//
    //FILE * outDesc = NULL;
    //string tmpName = outFileName+".tmp";
    //if((outDesc = fopen(tmpName.c_str(),"wb"))==NULL){
        //LOGG(Logger::ERROR) << "[BZip] Cannot open output file " <<outFileName << Logger::FLUSH;
        //return false;
    //}
//
    //int bzError;
    //BZFILE *bzf;
    //char buf[4096];
//
    //bzf = BZ2_bzReadOpen(&bzError, archiveDesc, 0, 0, NULL, 0);
    //if (bzError != BZ_OK) {
        //LOGG(Logger::ERROR) << "[BZip] Cannot initialize bzip on " <<infileName << Logger::FLUSH;
        //return false;
    //}
//
    //bool ret = true;
    //while (bzError == BZ_OK) {
        //int nread = BZ2_bzRead(&bzError, bzf, buf, sizeof buf);
        //if (bzError == BZ_OK || bzError == BZ_STREAM_END) {
            //size_t nwritten = fwrite(buf, 1, nread, outDesc);
            //if (nwritten != (size_t) nread) {
                //LOGG(Logger::ERROR) << "[BZip] Read and write differ " << infileName << Logger::FLUSH;
                //ret = false;
                //break;
            //}
        //}
    //}
//
    //if (bzError != BZ_STREAM_END) {
        //LOGG(Logger::ERROR) << "[BZip] Error after read " <<infileName << Logger::FLUSH;
        //ret = false;
    //}
    //fclose(outDesc);
    //// remove src file if neccesary
    //if(ret) {
       //if(removeSrc || infileName == outFileName) {
           //std::remove(infileName.c_str());
       //}
       //std::rename(tmpName.c_str(), outFileName.c_str());
    //} else {
        //// clean up if error
        //std::remove(tmpName.c_str());
    //}
//
    //BZ2_bzReadClose(&bzError, bzf);
    return true;
}
// void SnappyUtil::compress(const std::string &input, std::string &output) {
//     snappy::Compress(input.data(), input.size(), &output);
// }
// void SnappyUtil::decompress(const std::string& input, std::string &output) {
//     snappy::Uncompress(input.data(), input.size(), &output);
// }
