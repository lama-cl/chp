#!/bin/bash

WORK_DIR="$1"

if [ -z "${WORK_DIR}" ]; then
    WORK_DIR="${PWD}"
fi

echo "Building projects in ${WORK_DIR}"

for i in $(ls -1 ${WORK_DIR}); do
    echo "Configuring ${i}"
    cd "${WORK_DIR}/${i}"
    #if [ ! -e "configure" ] && [ -e "autogen.sh" ]
    #    ./autogen.sh
    #fi

    #if [ ! -e "configure" ] && [ -e "src/configure" ]
    #    cd src
    #fi

    #CC=clang CXX=clang++ ./configure
    #make clean
    #bear -- make
    #CC=clang CXX=clang++ make clean
    CC=clang CXX=clang++ bear -- make
done

