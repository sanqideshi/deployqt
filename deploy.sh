#!/bin/sh
appname=%1
dirname=`dirname $0`
tmp="${dirname#?}"

if [ "${dirname%$tmp}" != "/" ]; then
dirname=$PWD/$dirname
fi
LD_LIBRARY_PATH=$dirname/lib

cd $dirname

export LD_LIBRARY_PATH

$dirname/$appname "$@"
