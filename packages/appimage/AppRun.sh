#!/bin/sh

# AppImage runtime set `argv[0]` to AppImage file, which is not reliable.
# Qt framework expects reliable `argv[0]` to locate configuration files.
# This wrapper fixes `argv[0]`.

exec "$(dirname "$0")/usr/bin/RedPandaIDE" "$@"
