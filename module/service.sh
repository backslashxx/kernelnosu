#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
MODDIR=${0%/*}

USE_MOUNTIFY=false
. "$MODDIR/config.sh" 

if [ "$USE_MOUNTIFY" = "true" ]; then
	sh "$MODDIR/mountify_standalone.sh" 
fi

if [ -f "$MODDIR/system/bin/su" ]; then
	"$MODDIR/system/bin/su" --disable-sucompat
else
	"$MODDIR/system/product/bin/su" --disable-sucompat
fi
