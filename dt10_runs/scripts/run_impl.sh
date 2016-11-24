#!/bin/bash

NAME=$1;
SRC=$2;
RUN_NAME=$3;

mkdir -p impls
mkdir -p logs/${NAME};

LOG_DIR="logs/${NAME}/${RUN_NAME}";

mkdir -p ${LOG_DIR};

{
    echo "Log dir = ${LOG_DIR}";

    if [[ ! -d impls/${NAME} ]]; then

        echo "#### Cloning the implementation.";

        (cd impls && git clone ${SRC} ${NAME});
    fi;

    echo "### Pulling latest version.";

    (cd impls/${NAME} && git pull);

    echo "### Current git hash is:";

    git rev-parse HEAD impls/${NAME};

    echo "### Last commit message is:";

    (cd impls/${NAME} && git log -1);


    echo "#### Building lib-puzzler.";

    (cd impls/${NAME} && make lib/libpuzzler.a -B) | tee ${LOG_DIR}/build_lib_puzzler.log;

    echo "#### Building benchmark program and linking with lib-puzzler.";

    mkdir -p impls/${NAME}/bin;
    (cd impls/${NAME} && g++ -O3 -DNDEBUG=1 -g -std=c++11 -I include ../../bench_puzzle.cpp -o bin/bench_puzzle -L lib -pthread -l puzzler -lOpenCL -ltbb) > ${LOG_DIR}/build_bench_puzzler.log;

    BUDGET=30;
    WITH_SLACK=40;


    echo "#### Running circuit_timing";

    (cd impls/${NAME} && timeout ${WITH_SLACK} bin/bench_puzzle ising_spin 10 10 1.0 3 1 ../../${LOG_DIR}/ising_spin.user.log ../../${LOG_DIR}/ising_spin.csv ${BUDGET});

    echo "#### Running life";

    (cd impls/${NAME} && timeout ${WITH_SLACK} bin/bench_puzzle julia 10 10 1.4 3 1 ../../${LOG_DIR}/julia.user.log ../../${LOG_DIR}/julia.csv ${BUDGET});

    echo "#### Running brute_force";

    (cd impls/${NAME} && timeout ${WITH_SLACK} bin/bench_puzzle logic_sim 10 10 1.5 3 1 ../../${LOG_DIR}/logic_sim.user.log ../../${LOG_DIR}/logic_sim.csv ${BUDGET});

    echo "### Running matrix_exponent";

    (cd impls/${NAME} && timeout ${WITH_SLACK} bin/bench_puzzle random_walk 10 10 1.7 3 1 ../../${LOG_DIR}/random_walk.user.log ../../${LOG_DIR}/random_walk.csv ${BUDGET});

	echo "### Pruning log files";

	for i in ${LOG_DIR}/*.log; do
		cat $i | head -c 1000000 > $i.tmp;
		rm $i;
		mv $i.tmp $i;
	done

    echo "### Done";

} 2>&1 | tee ${LOG_DIR}/all.log;

(cd logs/${NAME} && tar -czf ${RUN_NAME}.tar.gz ${RUN_NAME});
