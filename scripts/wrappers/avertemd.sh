#!/bin/bash
# 
# File:   avertemd.sh
# Author: brett chaldecott
#
# Created on Jan 12, 2018, 5:44:49 PM
#

# this is only possible as root so holding thumbs it works
echo '/tmp/core.%h.%e.%t' > /proc/sys/kernel/core_pattern

# up the ulimits
ulimit -c unlimited
ulimit -a

# initialize the environment
export LC_ALL=en_US.UTF-8

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

# execute avertemd
exec ${KETO_HOME}/bin/avertemd $@


