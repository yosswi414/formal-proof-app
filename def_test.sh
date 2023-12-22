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

cat src/def_file | sed "s/\/\/.*//g" | sed "s/\\s//g" > src/def_file_nocomm
./test_automake3 src/def_file_nocomm ${def_name} script
./test_book3 src/def_file_nocomm script

echo "def_name = ${def_name}"
