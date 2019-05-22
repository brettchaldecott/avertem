#!/bin/bash

DIRECTORY=$1
TARGET=$2


echo tar -rvf ${DIRECTORY}.tar ${DIRECTOR} && cd ${TARGET} && tar -xf ${DIRECTORY}.tar && rm ${DIRECTORY}.tar
tar -rvf ${DIRECTORY}.tar ${DIRECTORY} && cd ${TARGET} && tar -xf ${DIRECTORY}.tar && rm ${DIRECTORY}.tar

