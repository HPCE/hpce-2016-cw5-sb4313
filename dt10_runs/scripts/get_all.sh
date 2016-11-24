#!/bin/bash

INPUT=inputs.csv
IFS=,

RUN_NAME=$(date -Ihours);

{
    read header;
    while read gitA gitB icA icB Name Team RepoLink
    do
        echo "Name : $Name";
        echo "RepoLink : $RepoLink";

        ./get_impl.sh ${Name} ${RepoLink}
    done
} < $INPUT
