#include <iostream>
#include <cstring>

#include "ext2fs.h"
#include "ext2fs_print.h"
#include "command.hpp"
#include "ed.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <image file> <command> <path>" << endl;
        return 1;
    }

    char *image_path(argv[1]), *command(argv[2]);

    if (strcmp(command, "mkdir") == 0) {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " <image file> mkdir <path>" << endl;
            return 1;
        }
        mkdir(image_path, argv[3]);
    } else if (strcmp(command, "rmdir") == 0) {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " <image file> rmdir <path>" << endl;
            return 1;
        }
        rmdir(image_path, argv[3]);
    } else if (strcmp(command, "rm") == 0) {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " <image file> rm <path>" << endl;
            return 1;
        }
        rm(image_path, argv[3]);
    } else if (strcmp(command, "ed") == 0) {
        if (argc != 10) {
            cerr << "Usage: " << argv[0] << " <image file> ed -i INDEX -b BACKSPACE /abs/path/to/file -- STRING" << endl;
            return 1;
        }
        ed(image_path, argv[3], ed_arguments(argc - 4, argv + 4));
    } else {
        cerr << "Unknown command: " << command << endl;
        return 1;
    }
}
