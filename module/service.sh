#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
MODDIR=${0%/*}

SU_BINARY="$(busybox find $MODDIR/system -name "su")"

# wait for boot-complete
until [ "$(getprop sys.boot_completed)" = "1" ]; do
	sleep 1
done

if [ -f "$SU_BINARY" ]; then
	[ -f "$MODDIR/escalate_prctl" ] && "$SU_BINARY" --disable-sucompat
	[ -f "$MODDIR/escalate_ioctl" ] && /data/adb/ksud feature set 0 0
fi

# EOF
