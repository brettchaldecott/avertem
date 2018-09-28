#!/bin/bash
# 
# File:   ketod.sh
# Author: brett chaldecott
#
# Created on Jan 12, 2018, 5:44:49 PM
#

# Get the current source directory
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Assume the source ends in /bin and strip that off
KETO_HOME=${SOURCE_DIR%%/bin}
export KETO_HOME

# upgrade
AUTO_UPGRADE=`cat ${KETO_HOME}/config/auto_upgrade`
if [ "${AUTO_UPGRADE}" == "1" ] ; then
    ${KETO_HOME}/upgrade/ubuntu.sh
fi

# execute ketod
${KETO_HOME}/bin/ketod $@


