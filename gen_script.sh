#!/bin/bash

if [ $# != 2 ]; then
    echo "Usage: $0 definition_name output_file"
    exit 2
fi

def_name=$1
out_name=$2

grep -F "${def_name}" src/def_file > /dev/null
if [ $? -eq 1 ]; then
    echo "No such name of definition found: \"${def_name}\""
    exit 1
fi

./test_automake3 src/def_file_nocomm ${def_name} ${out_name}
