#define CATCH_CONFIG_RUNNER

#include "catch.hpp"
#include "utilities.h"

int main(int argc, char* argv[]) {
    TMP_DB db;
    int result = Catch::Session().run( argc, argv );
    return ( result < 0xff ? result : 0xff );
}