#/bin/bash

#Build the passes
echo "Building liveness & reaching definition passes..."
make
echo "DONE building passes"

#Run the tests
echo "Building & running tests"
./build_and_dis_tests.sh
./do_liveness_tests.sh
./do_reaching_tests.sh
echo "DONE building & running tests"