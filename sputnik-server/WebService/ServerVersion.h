#pragma once

#include <string>

#ifdef SERVER_VERSION_STRING
# undef SERVER_VERSION_STRING
#endif
#ifdef SERVER_VERSION_NUMBER
# undef SERVER_VERSION_NUMBER
#endif

/*
** Compile-Time Library Version Numbers
**
** ^(The [SERVER_VERSION] C preprocessor macro in the header
** evaluates to a string literal that is the libsputnik version in the
** format "X.Y.Z" where X is the major version number 
** and Y is the minor version number and Z is the release number.)^
** ^(The [PARSER_VERSION_NUMBER] C preprocessor macro resolves to an integer
** with the value (X*1000000 + Y*1000 + Z) where X, Y, and Z are the same
** numbers used in [SERVER_VERSION].)^
*/

#define SERVER_VERSION_STRING	"1.0.0"
#define SERVER_VERSION_NUMBER 1
#define SERVER_SOURCE_ID	"2015-12-19 22:15:39.701"
#define SERVER_REVISION_ID	"b794faf5e8d3e88fb8c8199429a014803bbbb0be"

std::string server_version() {
    return SERVER_VERSION_STRING;
}

std::string server_source_id() {
    return SERVER_SOURCE_ID " " SERVER_REVISION_ID;
}

int server_version_number() {
    return SERVER_VERSION_NUMBER;
}
