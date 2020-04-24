/* Versions of OMP functions that can be used in sequential version */
#ifndef FAKE_OMP_H
int omp_get_max_threads();
int omp_get_num_threads();
int omp_get_thread_num();
void omp_set_num_threads(int num_threads);
#define FAKE_OMP_H 1
#endif
