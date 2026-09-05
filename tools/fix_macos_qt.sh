#!/bin/bash
TARGET=$1
if [ -z "$TARGET" ]; then exit 1; fi
for qtlib in QtWidgets QtGui QtCore QtNetwork QtSvg QtXml; do
    # Find the current path used in the binary for this Qt framework
    current_path=$(otool -L "$TARGET" | grep "$qtlib.framework" | awk '{print $1}')
    if [ ! -z "$current_path" ]; then
        install_name_tool -change "$current_path" "@executable_path/../Frameworks/$qtlib.framework/Versions/A/$qtlib" "$TARGET"
    fi
done
