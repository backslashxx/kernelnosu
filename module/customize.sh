#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
ELF_BINARY="su-arm"

if [ ! "$KSU" = true ]; then
	abort "[!] KernelSU only!"
fi

# this assumes CONFIG_COMPAT=y on CONFIG_ARM
arch=$(busybox uname -m)
echo "[+] detected: $arch"

case "$arch" in
	aarch64 | arm64 )
		ELF_BINARY="su-arm64"
		;;
	armv7l | armv8l )
		ELF_BINARY="su-arm"
		;;
	x86_64)
		ELF_BINARY="su-x64"
		;;
	*)
		abort "[!] $arch not supported!"
		;;
esac

# overlayfs ksu, /system/bin
# mksu, hunt for lowest filecount dir on $PATH

prep_system_bin() {
	mkdir -p "$MODPATH/system/bin"
	cp -f "$MODPATH/$ELF_BINARY" "$MODPATH/system/bin/su"
	busybox chcon --reference="/system/bin/sh" "$MODPATH/system/bin/su"
	chmod 755 "$MODPATH/system/bin/su"
	echo "[+] su will be on /system/bin"
}

prep_custom_dir() {
	line=$1
	if echo "$line" | grep -Eq "^/(product|vendor|odm|system_ext)/" && ! echo "$line" | grep -q "^/system/"; then
		line="/system$line"
	fi

	mkdir -p "$MODPATH/$line"
	cp -f "$MODPATH/$ELF_BINARY" "$MODPATH/$line/su"
	busybox chcon --reference="/system/bin/sh" "$MODPATH/$line/su"
	chmod 755 "$MODPATH/$line/su"
	echo "[+] su will be on $line/su"
}

# small snippet that hunts for folder in $PATH that has lowest number of files!
# IFS=":" ; for i in $PATH; do [ -d $i ] && find $i -type f | wc -l ; done ??
hunt_min_dir () {
IFS_old=$IFS
IFS=":" 
	min=99999
	min_dir=""

	for i in $PATH; do
		# https://github.com/5ec1cff/KernelSU/blob/main/userspace/ksud/src/installer.sh#L392
		# only allow magic mount-able paths
		echo "$i" | grep -qE "^/system/|^/vendor/|^/product/|^/system_ext/" || continue
		[ -d "$i" ] || continue

		count=$(busybox find "$i" -type f 2>/dev/null | wc -l)
		# debug
		echo "[-] $count $i"
		if [ "$count" -lt "$min" ]; then
			min=$count
			min_dir=$i
		fi
	done

	echo "[+] lowest file count dir: $min_dir ($min files)"
	
# reset IFS
IFS=$IFS_old

	prep_custom_dir "$min_dir"

}

if [ ! "$KSU_MAGIC_MOUNT" = "true" ]; then
	prep_system_bin
else
	hunt_min_dir
fi

SU_BINARY="$(busybox find $MODPATH/system -name "su")"
if [ -f "$SU_BINARY" ]; then
	"$SU_BINARY" --test-15 >/dev/null 2>&1 || abort "[!] Feature not implemented!"
fi

# EOF
