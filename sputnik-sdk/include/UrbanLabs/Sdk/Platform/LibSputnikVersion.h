#pragma once

#include <string>

#ifdef SPUTNIK_VERSION_STRING
# undef SPUTNIK_VERSION_STRING
#endif
#ifdef SPUTNIK_VERSION_NUMBER
# undef SPUTNIK_VERSION_NUMBER
#endif

/*
** Compile-Time Library Version Numbers
**
** ^(The [SPUTNIK_VERSION] C preprocessor macro in the header
** evaluates to a string literal that is the libsputnik version in the
** format "X.Y.Z" where X is the major version number 
** and Y is the minor version number and Z is the release number.)^
** ^(The [SPUTNIK_VERSION_NUMBER] C preprocessor macro resolves to an integer
** with the value (X*1000000 + Y*1000 + Z) where X, Y, and Z are the same
** numbers used in [SPUTNIK_VERSION].)^
*/

#define SPUTNIK_VERSION_STRING	"1.0.0"
#define SPUTNIK_VERSION_NUMBER 1
#define SPUTNIK_SOURCE_ID	"2017-03-11 22:49:17.202"
#define SPUTNIK_REVISION_ID	"083c07a6c2f8c21f640d5c47b1165d1e27df532e"

std::string libsputnik_version() {
    return SPUTNIK_VERSION_STRING;
}

std::string libsputnik_source_id() {
    return SPUTNIK_SOURCE_ID " " SPUTNIK_REVISION_ID;
}

int libsputnik_version_number() {
    return SPUTNIK_VERSION_NUMBER;
}
