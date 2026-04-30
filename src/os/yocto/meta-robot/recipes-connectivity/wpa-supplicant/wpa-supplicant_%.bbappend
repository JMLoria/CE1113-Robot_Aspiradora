FILESEXTRAPATHS:prepend := "${THISDIR}/wpa-supplicant:"

SRC_URI:append = " file://wpa_supplicant.conf"

do_install:append() {
    # Instalar el archivo de configuración con el nombre que systemd espera
    install -d ${D}${sysconfdir}/wpa_supplicant
    install -m 0600 ${WORKDIR}/wpa_supplicant.conf \
        ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf
}