#!/bin/sh
# bump the version and update the git tree accordingly
# !!!THIS SCRIPT IS FOR INTERNAL DEVELOPER USE ONLY!!!

type -P sed &>/dev/null || { echo "sed command not found. Aborting." >&2; exit 1; }
type -P git &>/dev/null || { echo "git command not found. Aborting." >&2; exit 1; }

if [ ! -n "$1" ]; then
  TAG=$(git describe --tags --abbrev=0 2>/dev/null)
  if [ ! -n "$TAG" ]; then
    echo Unable to read tag - aborting.
    exit 1
  fi
else
  TAG=$1
fi
if [ ! ${TAG:0:3} = 'cec' ]; then
  echo Tag "$TAG" does not start with 'cec' - aborting
  exit 1
fi
TAGVER=${TAG:3}
case $TAGVER in *[!0-9]*) 
  echo "$TAGVER is not a number"
  exit 1
esac
OFFSET=10000
TAGVER=`expr $TAGVER + 1`
TAGVER_OFF=`expr $TAGVER + $OFFSET`
echo "Bumping version to cec$TAGVER (nano: $TAGVER_OFF)"
sed -e "s/\(^m4_define(LIBCEC_NANO.*\)/m4_define(LIBCEC_NANO, [$TAGVER_OFF])/" configure.ac >> configure.ac~
mv configure.ac~ configure.ac
git commit -a -m "bumped internal version" -e
git tag "cec$TAGVER"
