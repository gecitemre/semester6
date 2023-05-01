#ifndef THREAD_ARGS_HPP
#define THREAD_ARGS_HPP

#include "matrix.hpp"

struct thread_args {
    int row;
    matrix *left, *right, *result;
};

#endif