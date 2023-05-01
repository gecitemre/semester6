#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include "hw2_output.h"

using namespace std;

struct thread_args;

extern sem_t **semaphores[2];

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
    void fill_stdin()
    {
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                cin >> mat[i][j];
            }
        }
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
    static void add(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, int mid, pthread_attr_t attr);
    static void multiply(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, int mid, pthread_attr_t attr);
};

struct thread_args
{
    matrix *left, *right, *result;
    int row, matrix_id;
    thread_args() {}
    thread_args(matrix *left, matrix *right, matrix *result, int row, int matrix_id) : left(left), right(right), result(result), row(row), matrix_id(matrix_id) {}
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
        sem_post(&semaphores[0][row][j]);
    }
    pthread_exit(NULL);
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
            int sval;
            sem_getvalue(&semaphores[0][targs->row][k], &sval);
            if (sval == 0)
            {
                sem_wait(&semaphores[0][targs->row][k]);
            }
            sum += row_left[k] * right->mat[k][j];
        }
        row_result[j] = sum;
        hw2_write_output(targs->matrix_id, targs->row, j, sum);
    }
    pthread_exit(NULL);
}

void matrix::add(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, int mid, pthread_attr_t attr)
{
    for (int i = 0; i < rows; i++)
    {
        pthread_create(&tids[i], &attr, add_row, new thread_args(&left, &right, &result, i, mid));
    }
}

void matrix::multiply(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, int mid, pthread_attr_t attr)
{
    for (int i = 0; i < rows; i++)
    {
        pthread_create(&tids[i], &attr, multiply_row, new thread_args(&left, &right, &result, i, mid));
    }
}

#endif