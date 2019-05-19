#!/bin/bash

DIRECTORY=$1
TARGET=$2


echo find ${DIRECTORY} -type f -regex ".*\.hpp\|.*\.h" -exec tar -rvf ${DIRECTORY}.tar {} \; && cd ${TARGET} && tar -xf ${DIRECTORY}.tar && rm ${DIRECTORY}.tar
find ${DIRECTORY} -type f -regex ".*\.hpp\|.*\.h" -exec tar -rvf ${DIRECTORY}.tar {} \; && cd ${TARGET} && tar -xf ${DIRECTORY}.tar && rm ${DIRECTORY}.tar

