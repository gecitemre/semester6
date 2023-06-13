#include <iostream>
#include <unistd.h>
#include "matrix.hpp"
#include "hw2_output.h"

using namespace std;

int main() {
    hw2_init_output();
    matrix A;
    A.fill_stdin();
    matrix B;
    B.fill_stdin();
    matrix C;
    C.fill_stdin();
    matrix D;
    D.fill_stdin();
    matrix J = A + B;
    matrix L = C + D;
    matrix R = J * L;
    cout << R;
}
