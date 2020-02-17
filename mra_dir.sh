#!/bin/bash

usage="Usage: mra_dir DIR [ARGS]\n\nExecute mra on all files in DIR, passing ARGS to mra\nDIR must be specified first."
ARG1=${1}

if [ $# -lt 2 -o "$ARG1" = "-h" ]; then
    echo $usage 
    exit 0
fi
shift
for filename in $ARG1/*.mra; do
  ./mra $* "$filename"
done

