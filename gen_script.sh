#!/bin/bash

if [ $# -lt 2 ] || [ $# -gt 3 ]; then
    echo "Usage: $0 definition_name output_file [def_file = nocomm]"
    exit 2
fi

def_name=$1
out_name=$2
defs_nocomm="src/def_file_nocomm"

if [ $# = 3 ]; then
    defs_nocomm=$3
fi

grep -F "${def_name}" ${defs_nocomm} > /dev/null
if [ $? = 1 ]; then
    echo "No such name of definition found: \"${def_name}\""
    exit 1
fi

./test_automake3 ${defs_nocomm} ${def_name} ${out_name}
