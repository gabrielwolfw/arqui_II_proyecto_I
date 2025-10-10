#include "test/ram_test.hpp"
#include <string>
#include <cstring>

int main(int argc, char* argv[]) {
    bool verbose = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
            break;
        }
    }

    return RAMTest::runTests(verbose) ? 0 : 1;
}