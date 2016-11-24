#!/bin/bash

NAME=$1
RUN=$2


DST=impls/${NAME};

DST_LOGS=${DST}/dt10_runs/${NAME};
echo "DST_LOGS = ${DST_LOGS}"

mkdir -p ${DST_LOGS};

(cd ${DST} && git pull);

echo "Copying logs"
ls logs/${NAME}
ls ${DST_LOGS}
cp logs/${NAME}/${RUN}.tar.gz ${DST_LOGS}/${RUN}.tar.gz;

echo "Copying graphs"
cp graphs/${NAME}/${RUN}/*.pdf ${DST_LOGS}/

PDFS=$(cd ${DST} && ls dt10_runs/${NAME}/*.pdf);

echo "PDFS = ${PDFS}";

(cd ${DST} && git add dt10_runs/${NAME}/${RUN}.tar.gz ${PDFS});
(cd ${DST} && git commit -m "Commit ${RUN}"  dt10_runs/${NAME}/${RUN}.tar.gz ${PDFS});
(cd ${DST} && git push);
