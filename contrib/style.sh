#!/bin/bash

# Copyright 2022, 2024 Hewlett Packard Enterprise Development LP
#
# run automatic style checkers and formatters

exec < /dev/tty

DIR="."

GIT_DIFF_CMD="git diff --no-ext-diff --cached HEAD"
GIT_DIFF_UNI_CMD="git diff --no-ext-diff -U0 --no-color --cached HEAD"
GIT_DIFF_NAME_CMD="git diff --no-ext-diff --name-only --cached HEAD"

CF_CMD="clang-format-11"
CF_DIFF_CMD="clang-format-diff-11 -p1"

CP_CMD=./contrib/checkpatch.pl
CP_OPTS="--no-tree --no-signoff --patch --summary-file --max-line-length=120 \
         --show-types --ignore IF_0,FILE_PATH_CHANGES,LINUX_VERSION_CODE,SPLIT_STRING"

line() {
  echo "======================================="
}

## CLANG-FORMAT

clangformat_check() {
  which ${CF_CMD} >/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    echo "ERROR: need to install ${CF_CMD}"
    exit 1
  fi
  VERSION=`${CF_CMD} --version | sed 's/[^0-9.]//g'`
  MAJOR=`echo ${VERSION} | cut -d. -f1`
  if [ -z "${MAJOR}" ] ; then
    echo "ERROR: empty clang-format version"
    exit 1
  fi
  if [ ${MAJOR} -lt 11 ] ; then
    echo "ERROR: wrong clang-format version (${VERSION})"
    exit 1
  fi
}

clangformat_run() {
  echo "Run clang-format..."

  FILES=`${GIT_DIFF_NAME_CMD} ${DIR} | grep -iE '\.(c|h)$'`
  if [ -z "${FILES}" ] ; then
    line
  fi
  for FILE in ${FILES} ; do

    line
    echo "Summary of changes for \"${FILE}\"..."
    CHANGES=`${GIT_DIFF_UNI_CMD} ${FILE} | ${CF_DIFF_CMD}`
    if [ -z "${CHANGES}" ] ; then
      echo "No changes for \"${FILE}\""
      continue
    fi
    echo "${CHANGES}"

    line
    echo "Action:  [I]gnore this file, [F]ix this file, [E]xit"
    read -r -n 1 -p "> " action
    echo

    case ${action} in
      "i" | "I")
        echo "Ignoring.."
        echo "To Fix, run \"${GIT_DIFF_UNI_CMD} ${FILE} | ${CF_DIFF_CMD} -i\""
        ;;
      "f" | "F")
        echo "Fixing..."
        ${GIT_DIFF_UNI_CMD} ${FILE} | ${CF_DIFF_CMD} -i
        git add ${FILE}
        if [ $? -ne 0 ] ; then
          echo "ERROR: clang-format failure"
          exit 1
        fi
        ;;
      *)
        echo "Exiting..."
        exit 1
        ;;
    esac

  done
  line
  echo "Done with clang-format"
}

## CHECKPATCH

checkpatch_check() {
  if [ ! -e ${CP_CMD} ] ; then
    echo "ERROR: can't find ${CP_CMD}"
    exit 1
  fi
  VERSION=`${CP_CMD} --version | grep Version | sed 's/[^0-9.]//g'`
  if [ -z "${VERSION}" ] ; then
    echo "ERROR: empty checkpatch version"
    exit 1
  fi
  if [ "${VERSION}" != "0.32" ] ; then
    echo "ERROR: wrong checkpatch version (${VERSION})"
    exit 1
  fi
}

checkpatch_run() {
  echo "Run checkpatch..."

  line
  ${GIT_DIFF_CMD} ${DIR} | ${CP_CMD} ${CP_OPTS}
  rtn=$?
  line
  if [ ${rtn} -ne 0 ] ; then
    echo "ERROR: checkpatch failed"
    exit 1
  fi
  echo "Done with checkpatch"
}

## MAIN

##clangformat_check
##clangformat_run
checkpatch_check
checkpatch_run
