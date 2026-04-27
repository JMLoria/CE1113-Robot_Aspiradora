FILESEXTRAPATHS:prepend := "${THISDIR}/wpa-supplicant:"

SRC_URI:append = " file://wpa_supplicant.conf"

do_install:append() {
    install -d ${D}${sysconfdir}
    install -m 0600 ${WORKDIR}/wpa_supplicant.conf ${D}${sysconfdir}/wpa_supplicant.conf

    # Habilitar servicio para wlan0
    install -d ${D}${sysconfdir}/wpa_supplicant
    ln -sf /lib/systemd/system/wpa_supplicant@.service \
        ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf
}