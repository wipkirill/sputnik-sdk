#pragma once 

#include <string>

#ifdef PARSER_VERSION_STRING
# undef PARSER_VERSION_STRING
#endif
#ifdef PARSER_VERSION_NUMBER
# undef PARSER_VERSION_NUMBER
#endif

/*
** Compile-Time Library Version Numbers
**
** ^(The [PARSER_VERSION] C preprocessor macro in the header
** evaluates to a string literal that is the libsputnik version in the
** format "X.Y.Z" where X is the major version number 
** and Y is the minor version number and Z is the release number.)^
** ^(The [PARSER_VERSION_NUMBER] C preprocessor macro resolves to an integer
** with the value (X*1000000 + Y*1000 + Z) where X, Y, and Z are the same
** numbers used in [PARSER_VERSION].)^
*/

#define PARSER_VERSION_STRING	"1.0.0"
#define PARSER_VERSION_NUMBER 1
#define PARSER_SOURCE_ID	"2017-03-11 22:49:17.433"
#define PARSER_REVISION_ID	"083c07a6c2f8c21f640d5c47b1165d1e27df532e"

std::string parser_version() {
    return PARSER_VERSION_STRING;
}

std::string parser_source_id() {
    return PARSER_SOURCE_ID " " PARSER_REVISION_ID;
}

int parser_version_number() {
    return PARSER_VERSION_NUMBER;
}
