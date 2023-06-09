#ifndef ED_HPP
#define ED_HPP

#include <iostream>
#include <cstring>

using namespace std;

struct ed_arguments {
    uint8_t index;
    uint8_t backspace;
    char* str;
    ed_arguments(int argc, char *argv[]);
};

#endif