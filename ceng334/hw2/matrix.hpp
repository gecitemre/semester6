#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include "hw2_output.h"

using namespace std;

struct thread_args;

extern sem_t **semaphores[2];
int count = -4;
class matrix
{
    int rows, cols;
    int **mat;
    int mid;
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
        mid = count++;
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
    static void add(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, pthread_attr_t attr);
    static void multiply(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, pthread_attr_t attr);
};

struct thread_args
{
    matrix *left, *right, *result;
    int row, matrix_id;
    thread_args() {}
    thread_args(matrix *left, matrix *right, matrix *result, int row) : left(left), right(right), result(result), row(row) {}
};

void *matrix::add_row(void *args)
{
    thread_args *targs = (thread_args *)args;
    matrix &left = *targs->left;
    matrix &right = *targs->right;
    matrix &result = *targs->result;
    int row = targs->row;
    int cols = left.cols;
    int* row_left = left.mat[row];
    int* row_right = right.mat[row];
    int* row_result = result.mat[row];
    int mid = result.mid;
    for (int j = 0; j < cols; j++)
    {
        row_result[j] = row_left[j] + row_right[j];
        hw2_write_output(mid, row+1, j+1, row_result[j]);
        sem_post(&semaphores[mid][row][j]);
    }
    pthread_exit(NULL);
}

void *matrix::multiply_row(void *args)
{
    thread_args *targs = (thread_args *)args;
    matrix &left = *targs->left;
    matrix &right = *targs->right;
    matrix &result = *targs->result;
    int **right_mat = right.mat;
    int row = targs->row;
    int* left_row = left.mat[row];
    int* result_row = result.mat[row];
    int cols = left.cols;
    for (int j = 0; j < right.cols; j++)
    {
        int sum = 0;
        for (int k = 0; k < cols; k++)
        {
            int sem_value;

            sem_wait(&semaphores[left.mid][row][k]);
            sem_post(&semaphores[left.mid][row][k]);
            sem_wait(&semaphores[right.mid][k][j]);
            sem_post(&semaphores[right.mid][k][j]);
            sum += left_row[k] * right_mat[k][j];
        }
        result_row[j] = sum;
        hw2_write_output(result.mid, row+1, j+1, sum);
    }
    pthread_exit(NULL);
}

void matrix::add(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, pthread_attr_t attr)
{
    for (int i = 0; i < rows; i++)
    {
        pthread_create(&tids[i], &attr, add_row, new thread_args(&left, &right, &result, i));
    }
}

void matrix::multiply(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, pthread_attr_t attr)
{
    for (int i = 0; i < rows; i++)
    {
        pthread_create(&tids[i], &attr, multiply_row, new thread_args(&left, &right, &result, i));
    }
}

#endif