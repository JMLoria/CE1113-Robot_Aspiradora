SUMMARY = "WiFi firmware symlinks para RPi5"
LICENSE = "CLOSED"

do_install() {
    install -d ${D}${nonarch_base_libdir}/firmware/brcm

    ln -sf brcmfmac43455-sdio.raspberrypi,5-compute-module.clm_blob \
        ${D}${nonarch_base_libdir}/firmware/brcm/brcmfmac43455-sdio.raspberrypi,5-model-b.clm_blob
}

FILES:${PN} = "${nonarch_base_libdir}/firmware/brcm/*"

RDEPENDS:${PN} = "linux-firmware-rpidistro-bcm43455"
