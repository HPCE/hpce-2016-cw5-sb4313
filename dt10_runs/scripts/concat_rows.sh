#!/bin/bash

for i in $(ls logs/*/*.tar.gz); do
    (cd $(dirname $i) && tar -xzf $(basename $i));
done


for i in $(ls logs/*/*/*.csv); do
    USER=$(echo $i | sed -r -e "s/logs\/([^\/]+).+/\1/g" -);
    RUN=$(echo $i | sed -r -e "s/logs\/[^\/]+\/([^\/]+).+/\1/g" -);
    
    if [[ -f impls/${USER}/dt10_runs/count_me_in ]]; then
        OPT_IN="1";
    else
        OPT_IN="0";
    fi;
    
    cat $i | {
        while read LINE; do
            echo "$USER, $RUN, ${OPT_IN}, $LINE";
        done
    }
done > all.rows;
