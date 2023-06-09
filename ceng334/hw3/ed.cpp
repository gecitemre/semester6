#include <iostream>
#include <cstring>

using namespace std;

struct ed_arguments {
    uint8_t index;
    uint8_t backspace;
    char* str;
    ed_arguments(int argc, char *argv[]){
        for (int i = 0; i < argc; i++) {
            char *arg = argv[i];
            if (strcmp(arg, "-i") == 0) {
                index = atoi(argv[i + 1]);
            } else if (strcmp(arg, "-b") == 0) {
                backspace = atoi(argv[i + 1]);
            } else if (strcmp(arg, "--") == 0) {
                str = argv[i + 1];
            }
        }
    }
};
