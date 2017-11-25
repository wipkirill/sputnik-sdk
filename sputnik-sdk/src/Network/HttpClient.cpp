// HttpClient.cpp
//

#ifdef COMPILE_HTTP_CLIENT

#include <cstring>

#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Network/HttpClient.h>

using namespace std;

#ifndef ANDROID
/**
 * @brief HttpClient::HttpClient
 * @param host
 */
HttpClient::HttpClient(const string &host)
    : host_(host) {
    ;
}
/**
 * @brief HttpClient::mapToRequest
 * @param req
 * @return
 */
string HttpClient::mapToRequest(const map <string, string> &req) const {
    bool ok = false;
    string res = "";
    for(auto r : req) {
        res = res+r.first+"="+r.second+"&";
        ok = true;
    }
    if(ok)
        res.resize(res.size()-1);
    return res;
}
/**
 * @brief HttpClient::writeToString
 * @param contents
 * @param size
 * @param nmemb
 * @param userp
 * @return
 */
size_t HttpClient::writeToString(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string *)(userp))->append((char *)contents, 0, size*nmemb);
    return size*nmemb;
}
/**
 * @brief HttpClient::waitOnSocket
 * @param sockfd
 * @param for_recv
 * @param timeout_ms
 * @return
 */
int HttpClient::waitOnSocket(curl_socket_t sockfd, int for_recv, long int timeout_ms) {
    struct timeval tv;
    fd_set infd, outfd, errfd;

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec= (timeout_ms % 1000) * 1000;

    FD_ZERO(&infd);
    FD_ZERO(&outfd);
    FD_ZERO(&errfd);

    FD_SET(sockfd, &errfd);
    if(for_recv)
        FD_SET(sockfd, &infd);
    else
        FD_SET(sockfd, &outfd);

    int res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
    return res;
}
/**
 * @brief HttpClient::send
 * @param curl
 * @param request
 * @param response
 * @return
 */
bool HttpClient::send(CURL *curl, const string &request, string &response) const {
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, host_.c_str());

        // Do not do the transfer - only connect to host
        //curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
        //CURLcode res = curl_easy_perform(curl);

        if(CURLE_OK != res) {
            LOGG(Logger::ERROR) << strerror(res) << Logger::FLUSH;
            return 1;
        }

        long sockextr;
        res = curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, &sockextr);

        if(CURLE_OK != res) {
            LOGG(Logger::ERROR) << curl_easy_strerror(res) << Logger::FLUSH;
            return 1;
        }

        curl_socket_t sockfd = sockextr;

        // wait for the socket to become ready for sending
        if(!waitOnSocket(sockfd, 0, 60000L)) {
            LOGG(Logger::ERROR) << "Error: timeout." << Logger::FLUSH;
            return false;
        }

        LOGG(Logger::INFO) << "Sending request." << Logger::FLUSH;

        size_t iolen;
        res = curl_easy_send(curl, request.c_str(), request.size(), &iolen);

        if(CURLE_OK != res) {
            LOGG(Logger::ERROR) << curl_easy_strerror(res) << Logger::FLUSH;
            return false;
        }
        LOGG(Logger::INFO) << "Reading response." << Logger::FLUSH;

        // read the response
        response = "";
        while(true) {
            // 1MB limit
            char buf[1024*1024];
            waitOnSocket(sockfd, 1, 60000L);
            res = curl_easy_recv(curl, buf, 1024*1024, &iolen);

            if(CURLE_OK != res) {
                break;
            } else {
                response = response+string(buf);
            }

            curl_off_t nread = (curl_off_t)iolen;
            LOGG(Logger::ERROR) << "Received " << nread << " bytes." << Logger::FLUSH;
        }
        curl_easy_cleanup(curl);
    }
    return true;
}
/**
 * @brief HttpClient::get
 * @param path
 * @param request
 * @param response
 * @return
 */
bool HttpClient::get(const string &path, const string &request, string &response) const {
    CURL *curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, (host_+"/"+path).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        return res == CURLE_OK;
    }
    return false;
}
/**
 * @brief HttpClient::get
 * @param path
 * @param request
 * @param response
 * @return
 */
bool HttpClient::get(const string &path, const map <string, string> request, string &response) const {
    return get(path+"?"+mapToRequest(request), "", response);
}
/**
 * @brief HttpClient::post
 * @param path
 * @param request
 * @param options
 * @param response
 * @return
 */
bool HttpClient::post(const string &path, const string &request, const map <string, string> options, string &response) const {
    CURL *curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, (host_+"/"+path).c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        struct curl_slist *headers = NULL;
        for(const auto opt : options) {
            headers = curl_slist_append(headers, (opt.first+": "+opt.second).c_str());
        }
        CURLcode res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        return res == CURLE_OK;
    }
    return false;
}
/**
 * @brief HttpClient::post
 * @param path
 * @param request
 * @param options
 * @param response
 * @return
 */
bool HttpClient::post(const string &path, const map <string, string> request, const map <string, string> options, string &response) const {
    return post(path, mapToRequest(request), options, response);
}
#endif // COMPILE_HTTP_CLIENT
#endif
