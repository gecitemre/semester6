#include <iostream>
#include "matrix.hpp"
#include "hw2_output.h"

using namespace std;

int main() {
    hw2_init_output();
    matrix A = matrix::from_stdin();
    matrix B = matrix::from_stdin();
    matrix C = matrix::from_stdin();
    matrix D = matrix::from_stdin();
    matrix J = A + B;
    cout << J;
    matrix L = C + D;
    cout << L;
    matrix R = J * L;
    cout << R;
}