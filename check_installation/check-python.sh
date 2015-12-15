#!/bin/sh

#
# Perform tests in ./python_tests. Before executing this script,
# run copy-python-tests.sh to create ./python_tests.
#

TESTDIR=python_tests

if [ ! -d "$TESTDIR" ]; then
    echo "ERROR: directory" $TESTDIR "does not exist, try running ./copy-python-tests.sh first."
    exit 1;
fi

echo "----------------------------------- "
echo "Testing Python bindings for HFST... "
echo "----------------------------------- "

echo ""
echo "Moving to directory" `pwd`"/"$TESTDIR"..."
echo ""
cd $TESTDIR

# ./test.sh --python python3
if ! (python3 test_hfst.py > /dev/null 2> /dev/null && python3 examples.py > /dev/null 2> /dev/null); then
    echo "FAIL: Python tests did not pass"
    echo ""
    echo "Exiting directory" `pwd`"..."
    echo ""

    cd ..
    exit 1
fi

echo "PASS: Python tests passed"
echo ""
echo "Exiting directory" `pwd`"..."
echo ""

cd ..



