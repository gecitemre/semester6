#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <iostream>
#include <pthread.h>
#include "hw2_output.h"

using namespace std;

struct thread_args;

int matrix_id;

class matrix
{
    int rows, cols;
    int **mat;
    static void *add_row(void *args);
    static void *multiply_row(void *args);
public:
    matrix(int rows, int cols) : rows(rows), cols(cols)
    {
        mat = new int *[rows];
        for (int i = 0; i < rows; i++)
        {
            mat[i] = new int[cols];
        }
    }
    static matrix from_stdin()
    {
        int rows, cols;
        cin >> rows >> cols;
        matrix m(rows, cols);
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                cin >> m.mat[i][j];
            }
        }
        return m;
    }
    matrix operator+(matrix &other);
    matrix operator*(matrix &other);
    friend ostream &operator<<(ostream &os, matrix &m)
    {
        for (int i = 0; i < m.rows; i++)
        {
            for (int j = 0; j < m.cols; j++)
            {
                os << m.mat[i][j] << " ";
            }
            os << endl;
        }
        return os;
    }
};

struct thread_args
{
    int row, matrix_id;
    matrix *left, *right, *result;
};

void *matrix::add_row(void *args)
{
    thread_args *targs = (thread_args *)args;
    int row = targs->row;
    int cols = targs->left->cols;
    int* row_left = targs->left->mat[row];
    int* row_right = targs->right->mat[row];
    int* row_result = targs->result->mat[row];
    for (int j = 0; j < cols; j++)
    {
        row_result[j] = row_left[j] + row_right[j];
        hw2_write_output(targs->matrix_id, row, j, row_result[j]);
    }
    pthread_exit(NULL);
}

matrix matrix::operator+(matrix &other)
{
    pthread_t threads[rows];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    matrix result(rows, cols);
    thread_args args[rows];
    for (int i = 0; i < rows; i++)
    {
        args[i].row = i;
        args[i].left = this;
        args[i].right = &other;
        args[i].result = &result;
        args[i].matrix_id = matrix_id;
        pthread_create(&threads[i], &attr, add_row, (void *)&args[i]);
    }
    matrix_id++;
    for (int i = 0; i < rows; i++)
    {
        pthread_join(threads[i], NULL);
    }
    return result;
}

matrix matrix::operator*(matrix &other) {
    pthread_t threads[rows];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    matrix result(rows, other.cols);
    thread_args args[rows];
    for (int i = 0; i < rows; i++)
    {
        args[i].row = i;
        args[i].left = this;
        args[i].right = &other;
        args[i].result = &result;
        args[i].matrix_id = matrix_id;
        pthread_create(&threads[i], &attr, multiply_row, (void *)&args[i]);
    }
    matrix_id++;
    for (int i = 0; i < rows; i++)
    {
        pthread_join(threads[i], NULL);
    }
    return result;
}

void *matrix::multiply_row(void *args)
{
    thread_args *targs = (thread_args *)args;
    int* row_left = targs->left->mat[targs->row];
    int* row_result = targs->result->mat[targs->row];
    int cols = targs->left->cols;
    matrix *right = targs->right;
    for (int j = 0; j < right->cols; j++)
    {
        int sum = 0;
        for (int k = 0; k < cols; k++)
        {
            sum += row_left[k] * right->mat[k][j];
            hw2_write_output(targs->matrix_id, targs->row, j, sum);
        }
        row_result[j] = sum;
    }
    pthread_exit(NULL);
}

#endif