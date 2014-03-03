#/bin/bash

#Build the passes
echo "Building LICM & DCE passes..."
make
echo "DONE building passes"

#Run the tests
echo "Building & running tests"
./build_and_dis_tests.sh
./do_licm_tests.sh
# ./do_deadcode_tests.sh
echo "DONE building & running tests"