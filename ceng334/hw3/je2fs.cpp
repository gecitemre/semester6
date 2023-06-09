#include <iostream>

#include "ext2fs.h"
#include "ext2fs_print.h"
#include "command.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <image file> <command> <path>" << endl;
        return 1;
    }

    string image_path(argv[1]), command(argv[2]);

    if (command == "mkdir") {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " <image file> mkdir <path>" << endl;
            return 1;
        }
        //mkdir(image_path, argv[3]);
    } else if (command == "rmdir") {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " <image file> rmdir <path>" << endl;
            return 1;
        }
        //rmdir(image_path, argv[3]);
    } else if (command == "rm") {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " <image file> rm <path>" << endl;
            return 1;
        }
        //rm(image_path, argv[3]);
    } else if (command == "ed") {
        //  ./je2fs FS_IMAGE ed -i INDEX -b BACKSPACE /abs/path/to/file -- STRING
        if (argc != 10) {
            cerr << "Usage: " << argv[0] << " <image file> ed -i INDEX -b BACKSPACE /abs/path/to/file -- STRING" << endl;
            return 1;
        }
        //ed(image_path, argv[3]);
    } else {
        cerr << "Unknown command: " << command << endl;
        return 1;
    }
}