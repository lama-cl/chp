#!/bin/bash

PROJ_DIR="."
if ! [ -z "$1" ]; then
    PROJ_DIR="$1"
fi

#for i in $(find ${PROJ_DIR} -maxdepth 2 -type f -name configure | cut -d '/' -f 2); do
for i in $(find ${PROJ_DIR} -maxdepth 1 -type d -name "[a-zA-Z]*" | cut -d '/' -f 2); do
    echo "Configuring ${i}"
    (
        LOG_FILE="${i}-build-compile-db.log"
        cd "${PROJ_DIR}/${i}"

        if [ ! -e "configure" ] && [ -e "autogen.sh" ]; then
            ./autogen.sh | tee -a ${LOG_FILE}
        fi

        if [ ! -e "configure" ] && [ -e "src/configure" ]; then
            cd src | tee -a ${LOG_FILE}
        fi

        CC=clang CXX=clang++ ./configure
        make clean | tee -a ${LOG_FILE}
        bear -- make | tee -a ${LOG_FILE}
        echo "End Status: $?" >> ${LOG_FILE}
    )
done

