#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <pthread.h>
#include <semaphore.h>
using namespace std;

class matrix
{
    int rows, cols;
    int **mat;
    int mid;
    static void *add_row(void *args);
    static void *multiply_row(void *args);
    pthread_t *tids;
    sem_t **semaphores;
    static int count;
    void wait();
    matrix(int rows, int cols);
    matrix(int rows, int cols, pthread_t *tids, sem_t **semaphores);
public:
    matrix();
    matrix(matrix &&other);
    ~matrix();
    void fill_stdin();
    matrix operator+(matrix &other);
    matrix operator*(matrix &other);
    friend ostream &operator<<(ostream &os, matrix &m);
    static void add(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, pthread_attr_t attr);
    static void multiply(matrix &left, matrix &right, matrix &result, int rows, int cols, pthread_t *tids, pthread_attr_t attr);
};

struct thread_args
{
    matrix *left, *right, *result;
    int row, matrix_id;
    thread_args();
    thread_args(matrix *left, matrix *right, matrix *result, int row);
};

#endif