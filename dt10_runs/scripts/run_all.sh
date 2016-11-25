#!/bin/bash

RUN_NAME="$1"

INPUT=inputs.csv
IFS=,

if [[ "" == "${RUN_NAME}" ]]; then
    echo "no run name"
    exit 1;
fi;

{
    read header;
    while read username gh
    do
        echo "Name : $username"

        if [[ ! -e logs/${username}/${RUN_NAME}.tar.gz ]]; then
            ./run_impl.sh ${username} git@github.com:HPCE/hpce-2016-cw5-${username}.git ${RUN_NAME}
        fi
    done
} < $INPUT
