/usr/bin/time -v -a -o tests_final_a_01_n_01.txt mpirun -n 1 ./run-epanet3 -h 24 -a 1 -l 1
/usr/bin/time -v -a -o tests_final_a_02_n_16.txt mpirun -n 16 ./run-epanet3 -h 24 -a 2 -l 9
/usr/bin/time -v -a -o tests_final_a_03_n_32.txt mpirun -n 16 ./run-epanet3 -h 24 -a 3 -l 9
