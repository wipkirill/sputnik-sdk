#include <cassert>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

using namespace std;

// a record separator for sqlite files
const string StringConsts::SEPARATOR = "|";
const string StringConsts::PT = ".";
const string StringConsts::COMMA = ",";
const string StringConsts::LINEBREAK = "\"\\/\b\f\n\r\t'";
//const string StringConsts::QUOTES = "”";
const string StringConsts::WHITESPACE = " \n\r\t";

/**
 * @brief SimpleTokenator::SimpleTokenator
 * @param line
 * @param delim
 * @param nonBreak
 * @param omit
 */
SimpleTokenator::SimpleTokenator(const string &line, char delim, char nonBreak, bool omit)
    : pos_(0), str_(line), delim_(delim), nonBreak_(nonBreak), omit_(omit)
{
    // TODO: this is not necessary?
    // the code that follows will behave normally
    if(!isValid()) {
        LOGG(Logger::WARNING) << "String line is not valid: " << line << Logger::FLUSH;
        throw std::exception();
    }

    // in case the delimeter is the space and we omit unnecessary
    // delimeters we make sure there is only one consequtive delimeter
    // in the string
    if(omit_ && delim == ' ') {
        int len = 0;
        str_ = StringUtils::trim(str_);
        for(int i = 0; len < str_.size() && i < str_.size(); i++) {
            if(str_[i] == delim_) {
                if(!(len > 0 && str_[len-1] == delim_)) {
                    str_[len] = str_[i];
                    len++;
                }
            } else {
                str_[len] = str_[i];
                len++;
            }
        }
        str_.resize(len);
    }

    brk_.push_back(-1);
    for(int i = 0, nbrk = 0; i < str_.size(); i++) {
        // toggle non break mode
        if(str_[i] == nonBreak)
            nbrk ^= 1;

        if(str_[i] == delim) {
            if(nbrk == 0) {
                brk_.push_back(i);
            }
        }
    }
    brk_.push_back(str_.size());
}
/**
 * @brief SimpleTokenator::isValid
 * @return
 */
bool SimpleTokenator::isValid() const {
    int count = 0;
    for(size_t i = 0; i < str_.size(); i++)
        if(str_[i] == nonBreak_)
            count++;
    return (count % 2) == 0;
}
/**
 * @brief isValid
 * @param str
 * @return
 */
bool SimpleTokenator::isValid(char nonBreak, const std::string &str) {
    int count = 0;
    for(size_t i = 0; i < str.size(); i++)
        if(str[i] == nonBreak)
            count++;
    return (count % 2) == 0;
}
/**
 * @brief SimpleTokenator::nextToken
 * @return
 */
string SimpleTokenator::nextToken() {
    if(pos_+1 >= brk_.size())
        throw std::exception();

    int len = brk_[pos_+1]-brk_[pos_]-1;
    if(len < 0)
        throw std::exception();

    string token = str_.substr(brk_[pos_]+1, len);
    pos_++;

    if(delim_ == ' ' && omit_)
        return StringUtils::trim(token);
    else
        return token;
}
/**
 * @brief SimpleTokenator::countTokens
 * @return
 */
size_t SimpleTokenator::countTokens() {
    // there should always be starting and ending break
    if(brk_.size() == 0)
        throw std::exception();
    return brk_.size()-1;
}
/**
 * @brief SimpleTokenator::getTokens
 * @return
 */
vector <string> SimpleTokenator::getTokens() {
    int nTokens = countTokens();
    vector<string> tokens;
    for(int i = 0; i < nTokens; i++) {
        tokens.push_back(nextToken());
    }
    reset();
    return tokens;
}
/**
 * @brief SimpleTokenator::reset
 */
void SimpleTokenator::reset() {
    pos_ = 0;
}
/**
 * @brief StringUtils::trim
 * @param s
 * @return
 */
string StringUtils::trim(const string& s) {
    return trimRight(trimLeft(s));
}
/**
 * @brief StringUtils::trimLeft
 * @param s
 * @return
 */
string StringUtils::trimLeft(const string& s) {
    size_t startpos = s.find_first_not_of(StringConsts::WHITESPACE);
    return (startpos == string::npos) ? "" : s.substr(startpos);
}
/**
 * @brief StringUtils::trimRight
 * @param s
 * @return
 */
string StringUtils::trimRight(const string& s) {
    size_t endpos = s.find_last_not_of(StringConsts::WHITESPACE);
    return (endpos == string::npos) ? "" : s.substr(0, endpos+1);
}
/**
 * @brief StringUtils::erase
 * @param s
 * @param toErase
 * @return
 */
string StringUtils::erase(string& s, const string &toEraseChars) {
    for(size_t i = 0; i < toEraseChars.size(); ++i) {
        s.erase(remove(s.begin(), s.end(), toEraseChars[i]), s.end());
    }
    return s;
}
/**
 * @brief StringUtils::isSpace
 * @param c
 * @return
 */
bool StringUtils::isSpace(char c){
    return isspace(c);
}
/**
 * @brief StringUtils::isNotspace
 * @param c
 * @return
 */
bool StringUtils::isNotspace(char c){
    return !isspace(c);
}
/**
 * @brief split
 * @param s
 * @return
 */
vector<string> StringUtils::split(const string& s) {
    typedef string::const_iterator iter;
    vector<string> ret;
    iter i = s.begin();
    while(i!=s.end()){
        // find the beginning of a word
        i = find_if(i, s.end(), isNotspace);
        // find the end of the same word
        iter j = find_if(i, s.end(), isSpace);
        if(i!=s.end()){
            ret.push_back(string(i, j));
            i = j;
        }
    }
    return ret;
}
/**
 * @brief StringUtils::filterWords
 * @param rawWords
 * @return
 */
vector<string> StringUtils::filterWords(const vector<string>& rawWords) {
    vector<string> result;
    for (const string& rawWord : rawWords) {
        string tmp = "";
        for (size_t i = 0; i < rawWord.size(); ++i) {
            if (!ispunct(rawWord[i])) {
                tmp += rawWord[i];
            }
        }
        if (!tmp.empty())
            result.push_back(tmp);
    }
    return result;
}
/**
 * @brief StringUtils::substr
 * @param s
 * @param length
 * @return
 */
string StringUtils::substr(const string& s, int length) {
    return s.substr(0, length);
}
/**
 * @brief StringUtils::escape
 * @param s
 * @return
 */
string StringUtils::quote(const string &s) {
    // remove trailing spaces
    string trimmed = StringUtils::trim(s);
    // erase linebrackes
    return "\""+StringUtils::replaceAll(StringUtils::erase(trimmed, StringConsts::LINEBREAK), "\"", "\"\"")+"\"";
}
/**
 * @brief StringUtils::escape
 * @param s
 * @return
 */
string StringUtils::escape(const string &s) {
    // remove trailing spaces
    string trimmed = StringUtils::trim(s);
    // erase linebrackes
    return StringUtils::erase(trimmed, StringConsts::LINEBREAK);
}
/**
 * @brief StringUtils::replaceAll
 * @param original
 * @param before
 * @param after
 * @return
 */
string StringUtils::replaceAll(const string &original, const string &before, const string &after) {
    string retval;
    string::const_iterator end     = original.end();
    string::const_iterator current = original.begin();
    string::const_iterator next    = search(current, end, before.begin(), before.end());
    while(next != end) {
        retval.append(current, next);
        retval.append(after);
        current = next+before.size();
        next = search(current, end, before.begin(), before.end());
    }
    retval.append(current, next);
    return retval;
}
/**
 * @brief base64_chars
 */
static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

/**
 * @brief base64_chars
 */
static const std::string base36_chars =
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";
/**
 * @brief is_base64
 * @param c
 * @return
 */
static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}
/**
 * @brief StringUtils::base64_encode
 * @param num
 * @return
 */
std::string StringUtils::base64_encode(int num) {
    string res = "";
    int len = base64_chars.size();
    do {
        res.append(1, base64_chars[num % len]);
        num /= len;
    } while(num > 0);
    return res;
}
/**
 * @brief StringUtils::base64_encode
 * @param num
 * @return
 */
std::string StringUtils::base36_encode(int num) {
    string res = "";
    int len = base36_chars.size();
    do {
        res.append(1, base36_chars[num % len]);
        num /= len;
    } while(num > 0);
    return res;
}
/**
 * @brief StringUtils::base64_encode
 * @param bytes_to_encode
 * @param in_len
 * @return
 */
std::string StringUtils::base64_encode(const char * bytes_to_encode, size_t in_len) {
    std::string ret;
    int i = 0, j = 0;
    char char_array_3[3], char_array_4[4];
    while(in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if(i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for(j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }
    return ret;

}
/**
 * @brief base64_decode
 * @param encoded_string
 * @return
 * TODO check if input string is valid base64 
 */
std::string StringUtils::base64_decode(std::string const& encoded_string) {
    int i = 0, j = 0, in_ = 0;
    int in_len = encoded_string.size();
    if(in_len % 4 != 0)
        throw std::runtime_error("Length of Base64 encoded input string is not a multiple of 4.");
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while(in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if(i == 4) {
            if (char_array_4[0] > 127 || char_array_4[1] > 127 || char_array_4[2] > 127 || char_array_4[3] > 127)
                throw std::runtime_error("Illegal character in Base64 encoded data.");
            for(i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            if (char_array_4[0] < 0 || char_array_4[1] < 0 || char_array_4[2] < 0 || char_array_4[3] < 0)
                throw std::runtime_error("Illegal character in Base64 encoded data.");

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for(i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }
    if(i) {
        for(j = i; j < 4; j++)
            char_array_4[j] = 0;

        for(j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for(j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }
    return ret;
}
/**
 * @brief stringToHex
 * @param data
 * @param len
 * @param out
 */
string StringUtils::stringToHex(const char* data, size_t len) {
    string out;
    const static char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    for( int i = 0; i < len; ++i )
    {
        char const bt = data[i];

        out += hex_chars[ ( bt & 0xF0 ) >> 4 ];
        out += hex_chars[ ( bt & 0x0F ) >> 0 ];
    }
    return out;
}
