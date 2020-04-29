#include <stdint.h>
#include <stdio.h>
#include <omp.h>
#include <CycleTimer.h>
#include <getopt.h>
#include <stdlib.h>
#include <algorithm>

#include "simulator.h"

int main(int argc, char **argv) {
	srand(15418);

	int num_threads = 1;
	string config_file_path = "";

	int opt;
	while ((opt = getopt(argc, argv, "t:p:")) != -1) {
		switch (opt) {
			case 't': {
				num_threads = atoi(optarg);
				break;
			}
			case 'p': {
				config_file_path = optarg;
				break;
			}
		}
	}

	omp_set_num_threads(num_threads);

	double start_time = CycleTimer::currentSeconds();
	Simulator test = Simulator(config_file_path);
	test.setup();
	test.simulate();
	double end_time = CycleTimer::currentSeconds();
	printf("Total Simulation Time in Secs: %f\n", end_time-start_time);

	return 0;
}


