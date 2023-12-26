#!/bin/bash

defs_nocomm="src/def_file_nocomm"

if [ $# -eq 1 ]; then
    defs_nocomm=$1
fi

cat src/def_file | sed "s/\/\/.*$//g" | sed "s/\\s//g" > ${defs_nocomm}
