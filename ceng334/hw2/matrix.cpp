#include <iostream>
#include "matrix.hpp"
#include "hw2_output.h"

using namespace std;

int matrix::count = -4;

void matrix::fill_stdin()
    {
        cin >> rows >> cols;
        mat = new int *[rows];
        for (int i = 0; i < rows; i++)
        {
            mat[i] = new int[cols];
            for (int j = 0; j < cols; j++)
            {
                cin >> mat[i][j];
            }
        }
    }
ostream &operator<<(ostream &os, matrix &m)
    {
        m.wait();
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
    
thread_args::thread_args() {}
thread_args::thread_args(matrix *left, matrix *right, matrix *result, int row) : left(left), right(right), result(result), row(row) {}

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
        sem_post(&result.semaphores[row][j]);
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
            sem_wait(&left.semaphores[row][k]);
            sem_post(&left.semaphores[row][k]);
            sem_wait(&right.semaphores[k][j]);
            sem_post(&right.semaphores[k][j]);
            sum += left_row[k] * right_mat[k][j];
        }
        result_row[j] = sum;
        hw2_write_output(result.mid, row+1, j+1, sum);
    }
    pthread_exit(NULL);
}

matrix matrix::operator+(matrix &other)
{
    pthread_t *tids = new pthread_t[rows];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    semaphores = new sem_t *[rows];
    for (int i = 0; i < rows; i++)
    {
        semaphores[i] = new sem_t[cols];
        for (int j = 0; j < cols; j++)
        {
            sem_init(&semaphores[i][j], 0, 0);
        }
    }
    matrix result(rows, cols, tids, semaphores);
    for (int i = 0; i < rows; i++)
    {
        pthread_create(&tids[i], &attr, add_row, new thread_args(this, &other, &result, i));
    }
    return result;
}

void matrix::wait()
{
    if (tids == NULL)
    {
        return;
    }
    for (int i = 0; i < rows; i++)
    {
        pthread_join(tids[i], NULL);
    }
    tids = NULL;
}

matrix::~matrix()
{
    wait();
}

matrix matrix::operator*(matrix &other)
{
    pthread_t *tids = new pthread_t[rows];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    matrix result(rows, other.cols, tids, semaphores);
    for (int i = 0; i < rows; i++)
    {
        pthread_create(&tids[i], &attr, multiply_row, new thread_args(this, &other, &result, i));
    }
    return result;
}


matrix::matrix() : matrix(0, 0) { }

matrix::matrix(int rows, int cols): matrix(rows, cols, NULL, NULL) { }

matrix::matrix(int rows, int cols, pthread_t *tids, sem_t **sems) : rows(rows), cols(cols), tids(tids), semaphores(sems) {
    mat = new int *[rows];
    for (int i = 0; i < rows; i++)
    {
        mat[i] = new int[cols];
    }
    mid = count++;
}