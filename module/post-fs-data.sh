#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
MODDIR=${0%/*}

USE_MOUNTIFY=false

[ -f "$MODDIR/config.sh" ] && . "$MODDIR/config.sh" 

if [ "$USE_MOUNTIFY" = "true" ]; then
	sh "$MODDIR/mountify_standalone.sh" 
fi

# EOF
