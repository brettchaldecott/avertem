#!/bin/bash
# 
# File:   avertemd.sh
# Author: brett chaldecott
#
# Created on Jan 12, 2018, 5:44:49 PM
#

#ulimit -c unlimited
#ulimit -a

# Get the current source directory
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Assume the source ends in /bin and strip that off
KETO_HOME=${SOURCE_DIR%%/bin}
export KETO_HOME

# execute avertemd
${KETO_HOME}/bin/avertem_rdf_tools "$@"



