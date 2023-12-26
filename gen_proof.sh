#!/bin/bash

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: $0 in_file [defs]"
    exit 2
fi

in_name=$1
defs_nocomm="src/def_file_nocomm"

if [ $# = 2 ]; then
    defs_nocomm=$2
fi

if [ ! -f ${in_name} ]; then
    echo "${in_name}: file not found"
    exit 127
fi

./test_book3 ${defs_nocomm} ${in_name}
