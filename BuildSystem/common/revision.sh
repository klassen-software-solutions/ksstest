#!/usr/bin/env bash

if git describe --tags --dirty=M >/dev/null 2>/dev/null; then
    newver=$(git describe --tags --dirty=M 2>/dev/null)
else
    newver="0.0.0"
fi

# allow override by environment variable
if [ x"$REVISION" != "x" ]; then
	newver=$REVISION
fi

if [ -f REVISION ]; then
    oldver=$(cat REVISION)
    if [ "$oldver" != "$newver" ]; then
        echo "$newver" > REVISION
    fi
else
    echo "$newver" > REVISION
fi

echo "$newver"
