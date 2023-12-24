#!/bin/bash

cat src/def_file | sed "s/\/\/.*$//g" | sed "s/\\s//g" > src/def_file_nocomm
