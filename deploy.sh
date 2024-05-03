#!/bin/sh
#appname=`basename $0 | sed s,\.sh$,,`
appname=%1
dirname=`dirname $0`
tmp="${dirname#?}"

if [ "${dirname%$tmp}" != "/" ]; then
dirname=$PWD/$dirname
fi
LD_LIBRARY_PATH=$dirname/lib
export LD_LIBRARY_PATH
#export QT_QPA_PLATFORM_PLUGIN_PATH=./plugins/platforms
$dirname/$appname "$@"
