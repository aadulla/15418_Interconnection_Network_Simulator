#include <stdint.h>
#include <stdio.h>
#include <omp.h>
#include <CycleTimer.h>
#include <getopt.h>

#include "simulator.h"

int main(int argc, char **argv) {
	int num_threads = 1;

	int opt;
	while ((opt = getopt(argc, argv, "t:")) != -1) {
		switch (opt) {
			case 't': {
				num_threads = atoi(optarg);
				break;
			}
		}
	}

	omp_set_num_threads(num_threads);

	double start_time = CycleTimer::currentSeconds();
	string config_file_path = "/afs/andrew.cmu.edu/usr3/aadulla/private/15418/15418_Interconnection_Network_Simulator/test_1/";
	Simulator test = Simulator(config_file_path);
	test.setup();
	test.simulate();
	double end_time = CycleTimer::currentSeconds();
	printf("Total Simulation Time in Secs: %f\n", end_time-start_time);

	return 0;
}


