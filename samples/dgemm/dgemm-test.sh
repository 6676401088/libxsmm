#!/bin/sh

LIBXSMM=../../lib/libxsmmld.so

HERE=$(cd $(dirname $0); pwd -P)
ECHO=$(which echo)
GREP=$(which grep)

${ECHO} "============================="
${ECHO} "Running DGEMM (ORIGINAL BLAS)"
${ECHO} "============================="
( time ${HERE}/dgemm-blas.sh $*; ) 2>&1 | ${GREP} real
${ECHO}

if [ -e dgemm-wrap ]; then
  ${ECHO} "============================="
  ${ECHO} "Running DGEMM (STATIC WRAP)"
  ${ECHO} "============================="
  ( time ${HERE}/dgemm-wrap.sh $*; ) 2>&1 | ${GREP} real
  ${ECHO}
fi

if [ -e ${LIBXSMM} ]; then
  ${ECHO} "============================="
  ${ECHO} "Running DGEMM (LD_PRELOAD)"
  ${ECHO} "============================="
  ( time LD_PRELOAD=${LIBXSMM} ${HERE}/dgemm-blas.sh $*; ) 2>&1 | ${GREP} real
fi

