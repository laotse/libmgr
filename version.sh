#!/bin/bash
#
# $Id: version.sh,v 1.4 2008-05-15 20:58:24 mgr Exp $
#
# Version string generation script
#
# Author:   Dr. Lars Hanke
# Created:  17.06.2004
#

# The file containing the version definition
VER_FILE="$1"
if test -z "$1"; then
    echo "Usage: $0 file [create|reset|report|build|revision|version]"
    echo "Mainatains a version tag: version.revision.build managed by"
    echo "#define _VERSION_ tags in file. The the tags will be changed"
    echo "in place."
    echo "reset action creates 1.1.1 version number"
    echo "report prints the _VERSION_ tag"
    echo "create creates the named version file with all supported tags"
    echo "Tags supported:"
    echo " _VERSION_       full tag string including author and date"
    echo " _VERSION_MAJOR_ numerical version"
    echo " _VERSION_MINOR_ numerical revision"
    echo " _VERSION_BUILD_ numerical build"
    echo "only the _VERSION_ tag is required, other tags are optional"
    exit 0
fi

function increment_build(){
    NVER="$1.$2.$(( $3 + 1 ))"
}

function increment_revision(){
    NVER="$1.$(( $2 + 1 )).1"
}

function increment_version(){
    NVER="$(( $1 + 1 )).1.1"
}

function set_numerical(){
    MAJOR="#define _VERSION_MAJOR_ $1"
    MINOR="#define _VERSION_MINOR_ $2"
    BUILD="#define _VERSION_BUILD_ $3"
}

TAGPAT="^[ \t]*#[ \t]*define[ \t][ \t]*_VERSION_"

if test ! -f "$VER_FILE"; then
  if test "$2"="create"; then
    echo "#define _VERSION_ \"\"" > "$VER_FILE"
    echo "#define _VERSION_MAJOR_ " >> "$VER_FILE"
    echo "#define _VERSION_MINOR_ " >> "$VER_FILE"
    echo "#define _VERSION_BUILD_ " >> "$VER_FILE"
  else
    echo "$VER_FILE does not exist!"
    exit 10;
  fi
else
  VSTR=`grep "${TAGPAT}[ \t]" "$VER_FILE" | sed -e "s/^.*\"\(.*\)\".*$/\1/g"`
  if [ -z "$VSTR" ]; then
    echo "No version string found - exiting!"
    NVER="1.1.1"
    exit 10
  fi
  VFLD=`echo $VSTR | sed -e "s/^\([0-9\.]*\).*/\1/g" | sed -e "s/\./ /g"`
fi

case "$2" in
  create|reset)
    NVER="1.1.1";;
  build)
    eval increment_build "$VFLD";;
  revision)
    eval increment_revision "$VFLD";;
  version)
    eval increment_version "$VFLD";;
  report)
    echo "$VSTR"
    exit 0;;
  *)
    echo "Unknwon action $2 - exiting"
    exit 10;;
esac

AUTH=`whoami`
DATE=`date +"%Y-%m-%d %H:%M"`
VOUT="#define _VERSION_ \"$NVER / $AUTH ($DATE)\"" 

VFLD=`echo $VOUT | sed -e "s/^.*\"\(.*\)\".*$/\1/g" | sed -e "s/^\([0-9\.]*\).*/\1/g" | sed -e "s/\./ /g"`

eval set_numerical $VFLD

cat "$VER_FILE" | sed -e "/${TAGPAT}[ \t]/c$VOUT" | sed -e "/${TAGPAT}BUILD_[ \t]/c$BUILD" | sed -e "/${TAGPAT}MAJOR_[ \t]/c$MAJOR" | sed -e "/${TAGPAT}MINOR_[ \t]/c$MINOR" > "$VER_FILE.$NVER"
mv -f "$VER_FILE.$NVER" "$VER_FILE"
echo "Advanced to $NVER / $AUTH in $VER_FILE"


