#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
MODDIR=${0%/*}

# drop small wrapper for pm for termux
# required when devpts hook is disabled
# from agnostic-apollo, https://github.com/termux/termux-packages/discussions/8292#discussioncomment-5102555

WRAPPER="IyEvYmluL3NoCm91dD0iJCgvc3lzdGVtL2Jpbi9wbSAiJEAiIDI+JjEgPC9kZXYvbnVsbCkiCmVjaG8gIiRvdXQiCiMgRU9GCg=="
TARGET="/data/data/com.termux/files/usr/bin/pm"

if [ -f "$TARGET" ] ; then
	# overwrite!
	echo "$WRAPPER" | busybox base64 -d > "$TARGET"
	
	if [ "$(echo $WRAPPER | busybox base64 -d | busybox crc32)" = "$(cat $TARGET | busybox crc32)" ]; then
		echo "[+] termux pm wrapper replaced!"
	else
		echo "[!] termux pm wrapper replace fail!"
	fi
else
	echo "[!] termux pm wrapper not found!"
fi

sleep 2

# EOF
