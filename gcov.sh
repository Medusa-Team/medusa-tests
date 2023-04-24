#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2023 by Matus Jokay

LOGNAME=logname
LCOV=lcov
GENHTML=genhtml
DEFINDIR=/sys/kernel/debug/gcov
DEFOUTDIR=gcov

# assume fulfilled dependencies (installed required utilities)
DEP_MET=1

# auxiliary functions

check() {
  PROG=`echo "${!1}"`
  PROG_PATH=`which "$PROG"`
  if [ x"$PROG_PATH" == x ]; then
    if [ x"$2" == x ]; then
      echo "Please install \`$PROG'."
    fi
    DEP_MET=0
  fi

  eval ${1}=$PROG_PATH
}

check_args() {
  while getopts ":ho:i:" arg; do
    case $arg in
      o) # Set output directory name.
	OUTDIR="${OPTARG}"
	;;
      i) # Set GCDA source directory.
	INDIR="${OPTARG}"
	;;
      * | h) # Display help.
	usage
	exit 0
	;;
    esac
  done

  if [ -z "${OUTDIR}" ]; then
    OUTDIR="${DEFOUTDIR}"
  fi

  if [ -z "${INDIR}" ]; then
    INDIR="${DEFINDIR}"
  fi
}

usage() {
  echo -e "Usage: $0 [-h] [-i <input_dir>] [-o <output_dir>]" >&2
  echo -e "\t-i: Set input GCDA directory name (default \`${DEFINDIR}')." >&2
  echo -e "\t-o: Set output directory name (default \`${DEFOUTDIR}')." >&2
  echo -e "\t-h: Print help." >&2
  exit 1
}

# sanity checks

# check arguments
[ $# -eq 0 ] && usage
check_args "${@}"

# check presence of utilities
check LOGNAME
check LCOV
check GENHTML "do_not_print_error_if_not_found"

if [ $DEP_MET -eq 0 ]; then
  exit 1
fi

# main program

# prepare Medusa's gcov dirs list
DIRS=`find "${INDIR}" -path '*/medusa/*' -type d 2>&1`
EPERM=`echo "${DIRS}" | grep -o "Permission denied"`
if [ x"${EPERM}" != x ] ; then
  echo "Please run as root."
  exit 1
fi

if [ x"${DIRS}" == x ]; then
  echo "Medusa GCOV profile is not set. Please turn it on and try again."
  exit 1
fi

# for each dir in the list create dir in output dir
rm -rf "${OUTDIR}"
OFS="${IFS}"
IFS='
'
for DIR in ${DIRS[@]}; do
  mkdir -p "${OUTDIR}/${DIR#*\/medusa\/}"
done
IFS="${OFS}"

# copy gcov data
find "${INDIR}" -path '*/medusa/*.gcda' -exec sh -c 'cat < $0 > '"${OUTDIR}"'/${0#*/medusa/}' {} \;
find "${INDIR}" -path '*/medusa/*.gcno' -exec sh -c 'cp -d $0 '"${OUTDIR}"'/${0#*/medusa/}' {} \;

# process lcov
TEMPDIR=`mktemp -d`
"${LCOV}" -c -d "${OUTDIR}" -o "${OUTDIR}"/lcov.info 1>/dev/null 2>&1
"${GENHTML}" -o "${TEMPDIR}" "${OUTDIR}"/lcov.info 1>/dev/null 2>&1
rm -rf "${OUTDIR}" 1>/dev/null 2>&1
mv "${TEMPDIR}" "${OUTDIR}" 1>/dev/null 2>&1
LOGNAME=`"${LOGNAME}"`
chown "${LOGNAME}":`id -Gn "${LOGNAME}" | cut -f1 -d' '` "${OUTDIR}" -R 1>/dev/null 2>&1

# final info
echo "HTML output is in \`"${OUTDIR}"' directory. Open \`"${OUTDIR}"/index.html' with a browser and enjoY!"

exit 0
