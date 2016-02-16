#!/bin/sh

HERE=$(cd $(dirname $0); pwd -P)
NAME=$(basename $0)

TOUCH=$(which touch)
ECHO=$(which echo)
SED=$(which sed)
TR=$(which tr)

DEST=$1
if [ "$1" = "" ]; then
  DEST=.
fi

SHELLFILE=${HERE}/${NAME}
STATEFILE=${DEST}/.state
STATE=$(${ECHO} $2 | ${TR} '?' '\n' | ${SED} -e 's/^  *//')

if [ ! -e ${STATEFILE} ] || [ 0 != $(${ECHO} "${STATE}" | diff -q ${STATEFILE} - > /dev/null; ${ECHO} $?) ]; then
  ${ECHO} "${STATE}" > ${STATEFILE}
  ${ECHO} ${SHELLFILE}
  ${TOUCH} ${SHELLFILE}
fi

