#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
MOUNTIFY_REQ=0
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

echo "#!/bin/sh" > "$MODPATH/config.sh"

test_mountify() {

	# routine start
	# echo "[+] mountify"
	# echo "[+] SysReq test"

	# test for overlayfs
	if grep -q "overlay" /proc/filesystems > /dev/null 2>&1; then \
		# echo "[+] CONFIG_OVERLAY_FS"
		# echo "[+] overlay found in /proc/filesystems"
		MOUNTIFY_REQ=$(( MOUNTIFY_REQ + 1))
	else
		# echo "[!] CONFIG_OVERLAY_FS test fail!"
		return
	fi

	# test for tmpfs xattr
	[ -w /mnt ] && MNT_FOLDER=/mnt
	[ -w /mnt/vendor ] && MNT_FOLDER=/mnt/vendor
	testfile="$MNT_FOLDER/tmpfs_xattr_testfile"
	rm "$testfile" > /dev/null 2>&1 
	busybox mknod "$testfile" c 0 0 > /dev/null 2>&1 
	if busybox setfattr -n trusted.overlay.whiteout -v y "$testfile" > /dev/null 2>&1 ; then 
		# echo "[+] CONFIG_TMPFS_XATTR"
		# echo "[+] tmpfs extended attribute test passed"
		rm "$testfile" > /dev/null 2>&1 
		MOUNTIFY_REQ=$(( MOUNTIFY_REQ + 1))
	else
		rm "$testfile" > /dev/null 2>&1 
		# echo "[!] CONFIG_TMPFS_XATTR test fail!"
		return
	fi

}

# for magic mount, we can test if setup can use mountify
# though we have to disable this on 3.x as old overlayfs 
# it does NOT have that selinux inherit thingy
if [ "$KSU_MAGIC_MOUNT" = "true" ] && [ -f "/data/adb/ksu/.nomount" ] &&
	[ ! "$(busybox uname -r | cut -d . -f1)" -lt 4 ]; then
	test_mountify
	
	if [ ! "$MOUNTIFY_REQ" = 2 ]; then
		abort "[!] please disable .nomount first!"
	fi
fi

if [ "$MOUNTIFY_REQ" = 2 ]; then
	echo "USE_MOUNTIFY=true" >> "$MODPATH/config.sh"
	echo "[+] mountify standalone script will mount su"
	touch "$MODPATH/skip_mount"
fi

# overlayfs ksu, /system/bin
# mksu + mountify /system/bin
# mksu - mountify, hunt for lowest filecount dir on $PATH

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

if [ ! "$KSU_MAGIC_MOUNT" = "true" ] || [ "$MOUNTIFY_REQ" = 2 ]; then
	prep_system_bin
else
	hunt_min_dir
fi

SU_BINARY="$(busybox find $MODPATH/system -name "su")"
if [ -f "$SU_BINARY" ]; then
	"$SU_BINARY" --test-15 >/dev/null 2>&1 || abort "[!] Feature not implemented!"
fi

# so mountify won't mount this always
touch "$MODPATH/skip_mountify"

# EOF
