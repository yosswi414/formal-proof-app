#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 in_file"
    exit 2
fi

in_name=$1

if [ ! -f ${in_name} ]; then
    echo "${in_name}: file not found"
    exit 127
fi

./test_book3 src/def_file_nocomm ${in_name}
