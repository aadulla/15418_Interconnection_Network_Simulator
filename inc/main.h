#ifndef MAIN_H
#define MAIN_H

#ifndef OMP
#define OMP 0
#endif

#if OMP
#include <omp.h>
#else
#include "fake_omp.h"
#endif

#endif /* MAIN_H */