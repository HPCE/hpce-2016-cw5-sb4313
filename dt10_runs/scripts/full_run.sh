#!/bin/bash

RUN_NAME=$(date -Ihours);

rm -rf logs
rm -rf graphs
rm -rf impls

./run_all.sh ${RUN_NAME}
./concat_rows.sh
python ./create_table.py ${RUN_NAME}
python ./plot_graphs.py ${RUN_NAME}
./commit_all.sh ${RUN_NAME}
