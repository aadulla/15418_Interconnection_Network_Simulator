#!/bin/bash

cd $PBS_O_WORKDIR
python3 ./src/test_suite_generator.py
python3 ./src/master.py 12