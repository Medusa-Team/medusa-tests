#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2023 by Matus Jokay

LCOV=lcov
GENHTML=genhtml
GCDA=/sys/kernel/debug/gcov
OUTDIR=.gcov

# assume fulfilled dependencies (installed lcov and genhtml)
DEP_MET=1

# auxiliary functions

check() {
  declare -n RET=$1

  PROG=`which "$1"`
  if [ x"$PROG" == x ]; then
    echo "Please install \`$1'."
    DEP_MET=0
  fi

  RET=$PROG
}

usage() {
  echo "Usage: $0 " >&2
  exit 1
}

# sanity checks

check $LCOV 
check $GENHTML

if [ $DEP_MET -eq 0 ]; then
  exit 1
fi

# main program

# prepare Medusa's gcov dirs list
DIRS=`find $GCDA -path '*/medusa/*' -type d 2>&1`
EPERM=`echo "$DIRS" | grep -o "Permission denied"`
if [ x"$EPERM" != x ] ; then
  echo "Please run as root."
  exit 1
fi

if [ x"$DIRS" == x ]; then
  echo "Medusa GCOV profile is not set. Please turn it on and try again."
  exit 1
fi

# for each dir in the list create dir in output dir
rm -rf ${OUTDIR}
OFS=${IFS}
IFS='
'
for DIR in ${DIRS[@]}; do
  mkdir -p ${OUTDIR}/${DIR#*\/medusa\/}
done
IFS=${OFS}

# copy gcov data
find $GCDA -path '*/medusa/*.gcda' -exec sh -c 'cat < $0 > '$OUTDIR'/${0#*/medusa/}' {} \;
find $GCDA -path '*/medusa/*.gcno' -exec sh -c 'cp -d $0 '$OUTDIR'/${0#*/medusa/}' {} \;

# process lcov
TEMPDIR=`mktemp -d`
$LCOV -c -d ${OUTDIR} -o ${OUTDIR}/lcov.info 1>/dev/null 2>&1
$GENHTML -o ${TEMPDIR} ${OUTDIR}/lcov.info 1>/dev/null 2>&1
rm -rf ${OUTDIR}
mv ${TEMPDIR} ${OUTDIR}
chown medusa:medusa ${OUTDIR}  -R

# final info
echo "HTML output is in \`${OUTDIR}' directory. Open \`${OUTDIR}/index.html' with a browser and enjoY!"

exit 0
