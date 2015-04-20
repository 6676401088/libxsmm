#!/bin/bash

VARIANT=Specialized

if ([ "" != "$1" ]) ; then
  VARIANT=$1
fi

HERE=$(cd $(dirname $0); pwd -P)
PERF=$(grep -A1 -i "${VARIANT}" ${HERE}/smm-test.txt | \
  grep -e "performance" | \
  cut -d" " -f2 | \
  sort -n)

NUM1=$(echo "${PERF}" | wc -l)
NUM2=$((NUM1 / 2))
MIN=$(echo ${PERF} | cut -d" " -f1)
MAX=$(echo ${PERF} | cut -d" " -f${NUM1})
AVG=$(echo "$(echo -n "scale=3;(${PERF})/${NUM1}" | tr "\n" "+")" | bc)

if ([ "0" == "$((NUM1 % 2))" ]) ; then
  A=$(echo ${PERF} | cut -d" " -f$((NUM2 - 1)))
  B=$(echo ${PERF} | cut -d" " -f${NUM2})
  MED=$(echo "$(echo -n "scale=3;(${A} + ${B})/2")" | bc)
else
  MED=$(echo ${PERF} | cut -d" " -f${NUM2})
fi

echo "num=${NUM1}"
echo "min=${MIN}"
echo "max=${MAX}"
echo "avg=${AVG}"
echo "med=${MED}"

