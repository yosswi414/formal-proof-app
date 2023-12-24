#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 definition_name"
    exit 2
fi

def_name=$1

grep -F "${def_name}" src/def_file > /dev/null
if [ $? -eq 1 ]; then
    echo "No such name of definition found: \"${def_name}\""
    exit 1
fi

./trim_comment.sh
status=$?
if [ ${status} != 0 ]; then
    exit ${status}
fi
./gen_script.sh ${def_name} script
status=$?
if [ ${status} != 0 ]; then
    exit ${status}
fi
./gen_proof.sh script
status=$?
if [ ${status} != 0 ]; then
    exit ${status}
fi

echo "def_name = ${def_name} seems valid."
