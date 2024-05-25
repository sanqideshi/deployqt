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
export QT_QPA_PLATFORM_PLUGIN_PATH=$dirname/plugins/platforms
#export QML2_IMPORT_PATH=$dirname/qml
#export QTWEBENGINEPROCESS_PATH=$dirname/libexec/QtWebEngineProcess
#export QTWEBENGINE_RESOURCES_PATH=$dirname/resources
#export QTWEBENGINE_LOCALES_PATH=$dirname/translations/qtwebengine_locales
export QT_QPA_PLATFORM=xcb
$dirname/$appname "$@"
