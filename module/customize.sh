DEBUG=false

chcon u:object_r:system_file:s0 "$MODPATH/system/bin/su"
chmod 755 "$MODPATH/system/bin/su"
