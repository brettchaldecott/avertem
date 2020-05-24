#!/bin/bash

LIBRARY=$2
LIBRARY=${LIBRARY#*keto_}
SEARCH_PATH=$1/$LIBRARY
MODULAS=$3

SCLIST=0
HCLIST=0
for key in "$@" ; do
    case $key in
        -sc)
            SCLIST=1
            ;;
        -hc)
            HCLIST=1
            ;;
    esac
    shift # past argument or value
done


if [ ! -e $SEARCH_PATH ]
then
    exit 0
fi

OS="$(uname -s)"
if [ -d "${SEARCH_PATH}" ] && [ "${OS}" == "Linux" ] ;
then
    SOURCE_CLASSES=(`find $SEARCH_PATH -name "*.hpp" | xargs grep -l getSourceVersion | sed "s/include/#/g" | cut -f 2 -d "#" - | cut -f 1 -d ":" - | cut -f 1 -d "." - | cut -f 2-4 -d "/" --output-delimiter='::' -`)
    HEADER_CLASSES=(`find $SEARCH_PATH -name "*.hpp" | xargs grep -l getHeaderVersion | sed "s/include/#/g" | cut -f 2 -d "#" - | cut -f 1 -d ":" - | cut -f 1 -d "." - | cut -f 2-4 -d "/" --output-delimiter='::' -`)
else
    SOURCE_CLASSES=()
    HEADER_CLASSES=()
fi

COUNT_HEADERS=0
if [ $HCLIST == 1 ]
then
    for classEntry in "${HEADER_CLASSES[@]}";
    do
       HEADER_MODULAS=$((${COUNT_HEADERS} % ${MODULAS}))
       if [ ${HEADER_MODULAS} -eq 0 ] ; then
            echo "hash = generateHash(avertemConcat(hash,getSourceVersion(\"$classEntry::getHeaderVersion\")));"
       fi
       COUNT_HEADERS=$((COUNT_HEADERS+1))
    done
fi

COUNT_SOURCE=0
if [ $SCLIST == 1 ]
then
    for classEntry in "${SOURCE_CLASSES[@]}";
    do
       SOURCE_MODULAS=$((${COUNT_SOURCE} % ${MODULAS}))
       if [ ${SOURCE_MODULAS} -ne 0 ] ; then
            echo "hash = generateHash(avertemConcat(hash,getSourceVersion(\"$classEntry::getHeaderVersion\")));"
       fi
       COUNT_SOURCE=$((COUNT_SOURCE+1))
    done
fi
