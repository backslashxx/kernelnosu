#!/bin/sh
PATH=/data/adb/ksu/bin:$PATH
MOUNTIFY_REQ=0

# subject to change ofcourse
if [ ! "$KSU" = true ] || [ ! "$KSU_KERNEL_VER_CODE" -ge 12040 ]; then
	abort "[!] KernelSU 12040+ required!"
fi

arch=$(busybox uname -m)
if [ "$arch" = "aarch64" ] || [ "$arch" = "armv7l" ] || [ "$arch" = "armv8l" ]; then
	echo "[+] detected: $arch"
else
	abort "[!] $arch not supported!"
fi


test_mountify() {
	# routine start
	echo "[+] mountify"
	echo "[+] SysReq test"

	# test for overlayfs
	if grep -q "overlay" /proc/filesystems > /dev/null 2>&1; then \
		echo "[+] CONFIG_OVERLAY_FS"
		echo "[+] overlay found in /proc/filesystems"
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
		echo "[+] CONFIG_TMPFS_XATTR"
		echo "[+] tmpfs extended attribute test passed"
		rm "$testfile" > /dev/null 2>&1 
		MOUNTIFY_REQ=$(( MOUNTIFY_REQ + 1))
	else
		rm "$testfile" > /dev/null 2>&1 
		# echo "[!] CONFIG_TMPFS_XATTR test fail!"
		return
	fi

}

if [ "$KSU_MAGIC_MOUNT" = "true" ]; then
	test_mountify
fi

if [ "$MOUNTIFY_REQ" = 2 ]; then
	echo "USE_MOUNTIFY=true" > "$MODPATH/config.sh"
	touch "$MODPATH/skip_mount"
fi

# overlayfs ksu, /system/bin
# mksu + mountify /system/bin
# mksu - mountify /product/bin if it has /product/bin, otherwise /system/bin

prep_system_bin() {
	mv -f "$MODPATH/su" "$MODPATH/system/bin/su"
	rm -rf "$MODPATH/system/product"
	busybox chcon --reference="/system/bin/sh" "$MODPATH/system/bin/su"
	chmod 755 "$MODPATH/system/bin/su"
}

prep_product_bin() {
	mv -f "$MODPATH/su" "$MODPATH/system/product/bin/su"
	rm -rf "$MODPATH/system/bin"
	busybox chcon --reference="/system/bin/sh" "$MODPATH/system/product/bin/su"
	chmod 755 "$MODPATH/system/product/bin/su"
}

if { [ "$KSU_MAGIC_MOUNT" = "true" ] && [ "$MOUNTIFY_REQ" = 2 ]; } || [ ! "$KSU_MAGIC_MOUNT" = "true" ]; then
	prep_system_bin
else
	if [ -d "/product/bin" ] || [ -d "/system/product/bin" ]; then
		prep_product_bin
	else
		prep_system_bin
	fi
	
fi

# so mountify won't mount this always
touch "$MODPATH/skip_mountify"

# EOF
