#!/bin/bash

NAME=$1;
SRC=$2;

mkdir -p impls

if [[ ! -d impls/${NAME} ]]; then

    echo "#### Cloning the implementation.";

    (cd impls && git clone ${SRC} ${NAME});
fi;

echo "### Pulling latest version.";

(cd impls/${NAME} && git pull);
