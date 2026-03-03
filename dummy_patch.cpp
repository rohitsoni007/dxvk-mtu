#include <iostream>
#include <string>
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        std::cout << "patch 2.7.6" << std::endl;
    }
    return 0;
}
