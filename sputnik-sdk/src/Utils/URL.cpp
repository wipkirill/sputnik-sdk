#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Utils/URL.h>

using std::string;

// scheme://domain:port/path?query#fragment
URL::URL(const string &url) : port(-1), scheme_(""), domain_(""), path_(""), query_(""), frag_("") {
    path_ = url;
}

int URL::getPort() const {
    return port;
}

string URL::getScheme() const {
    return scheme_;
}

string URL::getResource() const {
    return domain_;
}

string URL::getPath() const {
    return path_;
}

string URL::getQuery() const {
    return query_;
}

string URL::getFragment() const {
    return frag_;
}
