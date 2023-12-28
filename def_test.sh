#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 definition_name"
    exit 2
fi

def_name=$1
script_name="_script_def_test"
defs_nocomm="src/_def_file_nocomm"

function finally () {
    rm -f ${script_name}
    rm -f ${defs_nocomm}
}

trap finally EXIT   # run finally() before exiting except receiving SIGKILL

./trim_comment.sh ${defs_nocomm}

grep "^${def_name}$" ${defs_nocomm} > /dev/null
if [ $? -eq 1 ]; then
    echo "No such name of definition found: \"${def_name}\""
    exit 1
fi

status=$?
if [ ${status} != 0 ]; then
    exit ${status}
fi
./gen_script.sh ${def_name} ${script_name} ${defs_nocomm} > /dev/null
status=$?
if [ ${status} != 0 ]; then
    exit ${status}
fi
./gen_book.sh ${script_name} ${defs_nocomm} > /dev/null
status=$?
if [ ${status} != 0 ]; then
    exit ${status}
fi

# echo "def_name = ${def_name} seems valid."
