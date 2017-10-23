#include <iostream>

// Print builder diagnostic information
int main(int argc, char **argv) {
    std::cout<<"# builds running";
    std::cout<<"# builds queued";
    std::cout<<"# builds finished";
    std::cout<<"# builds killed";
    std::cout<<"# resources active out of a possible #";

    return 0;
}