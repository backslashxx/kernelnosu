#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
MODDIR=${0%/*}

SU_BINARY="$(busybox find $MODDIR/system -name "su")"

# wait for boot-complete
until [ "$(getprop sys.boot_completed)" = "1" ]; do
	sleep 1
done

[ -f "$SU_BINARY" ] && "$SU_BINARY" --disable-sucompat

# EOF
