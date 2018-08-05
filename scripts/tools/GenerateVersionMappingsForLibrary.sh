#!/bin/bash

LIBRARY=$2
LIBRARY=${LIBRARY#*keto_}
SEARCH_PATH=$1/$LIBRARY

HEADERS=0
SCLIST=0
HCLIST=0
for key in "$@" ; do
    case $key in
        -h)
            HEADERS=1
            ;;
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


HEADER_FILES=(`find $SEARCH_PATH -name "*.hpp" | xargs grep -l getHeaderVersion | sed "s/include/#/g" | cut -f 2 -d "#" - | cut -f 2-4 -d "/" -`)
SOURCE_CLASSES=(`find $SEARCH_PATH -name "*.hpp" | xargs grep -l getSourceVersion | sed "s/include/#/g" | cut -f 2 -d "#" - | cut -f 1 -d ":" - | cut -f 1 -d "." - | cut -f 2-4 -d "/" --output-delimiter='::' -`)
HEADER_CLASSES=(`find $SEARCH_PATH -name "*.hpp" | xargs grep -l getHeaderVersion | sed "s/include/#/g" | cut -f 2 -d "#" - | cut -f 1 -d ":" - | cut -f 1 -d "." - | cut -f 2-4 -d "/" --output-delimiter='::' -`)

if [ $HEADERS == 1 ]
then
    for headerEntry in "${HEADER_FILES[@]}";
    do
       echo "#include \"$headerEntry\""
    done
fi

if [ $HCLIST == 1 ]
then
    for classEntry in "${HEADER_CLASSES[@]}";
    do
       echo "      sourceVersionMap[\"$classEntry::getHeaderVersion\"] = &$classEntry::getHeaderVersion;"
    done
fi

if [ $SCLIST == 1 ]
then
    for classEntry in "${SOURCE_CLASSES[@]}";
    do
       echo "      sourceVersionMap[\"$classEntry::getSourceVersion\"] = &$classEntry::getSourceVersion;"
    done
fi
