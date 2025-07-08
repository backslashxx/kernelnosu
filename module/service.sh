#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
MODDIR=${0%/*}

USE_MOUNTIFY=false
SU_BINARY="$(busybox find $MODDIR/system -name "su")"

[ -f "$MODDIR/config.sh" ] && . "$MODDIR/config.sh" 

# wait for boot-complete
until [ "$(getprop sys.boot_completed)" = "1" ]; do
	sleep 1
done

if [ "$USE_MOUNTIFY" = "true" ]; then
	sh "$MODDIR/mountify_standalone.sh" 
fi

[ -f "$SU_BINARY" ] && "$SU_BINARY" --disable-sucompat


# EOF
