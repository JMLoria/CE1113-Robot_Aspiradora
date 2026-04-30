FILESEXTRAPATHS:prepend := "${THISDIR}/linux-raspberrypi:"
SRC_URI:append = " file://wifi.cfg"
KERNEL_CONFIG_FRAGMENTS:append = " ${WORKDIR}/wifi.cfg"
