#!/bin/bash

INPUT=inputs.csv
IFS=,

RUN_NAME=$1;

if [[ "" == "${RUN_NAME}" ]]; then
	exit 1;
fi;

{
    read header;
    while read ic gh
    do
        echo "Name : ${ic}";

	./commit_back.sh ${ic} ${RUN_NAME};

        ##if [[ ! -e logs/${Name}/${RUN_NAME}.tar.gz ]]; then
        #    ./run_impl.sh ${Name} ${RepoLink}
        #fi
    done
} < $INPUT
