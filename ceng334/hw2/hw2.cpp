#include <iostream>
#include "matrix.hpp"
#include "hw2_output.h"
#include <unistd.h>

using namespace std;

sem_t **semaphores[2];
int main() {
    hw2_init_output();
    int N, M, K;
    cin >> N >> M;
    matrix A(N, M);
    A.fill_stdin();
    cin >> N >> M;
    matrix B(N, M);
    B.fill_stdin();
    cin >> M >> K;
    matrix C(M, K);
    C.fill_stdin();
    cin >> M >> K;
    matrix D(M, K);
    D.fill_stdin();
    semaphores[0] = new sem_t *[N];
    for (int i = 0; i < N; i++)
    {
        semaphores[0][i] = new sem_t[M];
        sem_init(semaphores[0][i], 0, 0);
    }
    semaphores[1] = new sem_t *[M];
    for (int i = 0; i < M; i++)
    {
        semaphores[1][i] = new sem_t[K];
        sem_init(semaphores[1][i], 0, 0);
    }
    int thread_count = N + M + K;
    pthread_t tids[thread_count];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    thread_args args[thread_count];
    matrix J(N, M);
    matrix::add(A, B, J, N, M, tids, attr);
    matrix L(M, K);
    matrix::add(C, D, L, M, K, tids + N, attr);
    matrix R(N, K);
    matrix::multiply(J, L, R, N, K, tids + N + M, attr);
    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(tids[i], NULL);

    }
    cout << R;
}