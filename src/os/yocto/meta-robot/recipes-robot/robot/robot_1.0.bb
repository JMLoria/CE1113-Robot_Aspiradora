SUMMARY = "Proyecto Integrador - Robot Aspiradora"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

PR = "r9"

DEPENDS = "boost libgpiod alsa-lib mpg123"
RDEPENDS:${PN} = "mpg123 alsa-utils"

SRC_URI = "file://CMakeLists.txt \
           file://librobot \
           file://int \
           file://robot.service \
           file://asound.conf"

S = "${WORKDIR}"

inherit cmake pkgconfig systemd

SYSTEMD_SERVICE:${PN} = "robot.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    # 1. Directorios base
    install -d ${D}${libdir}
    install -d ${D}${bindir}
    install -d ${D}${sysconfdir}
    install -d ${D}${systemd_system_unitdir}

    # 2. Binario, Librería y Config de Audio
    install -m 0755 ${B}/int/robot_int_serv ${D}${bindir}/
    install -m 0755 ${B}/librobot/librobot.so ${D}${libdir}/
    install -m 0644 ${WORKDIR}/asound.conf ${D}${sysconfdir}/asound.conf

    # 3. Estructura en /home (Web y Data)
    mkdir -p ${D}/home/web
    mkdir -p ${D}/home/data/r_users
    mkdir -p ${D}/home/data/musica
    mkdir -p ${D}/home/data/sonidos

    # 4. Instalación de archivos Web
    if [ -d ${S}/int/web ]; then
        cp -r ${S}/int/web/* ${D}/home/web/
    fi

    # 5. Instalación de Datos (JSON y Música)
    if [ -f ${S}/int/data/r_users/users_robot.json ]; then
        cp ${S}/int/data/r_users/users_robot.json ${D}/home/data/r_users/
    fi

    if [ -d ${S}/int/data/musica ]; then
        cp -r ${S}/int/data/musica/* ${D}/home/data/musica/ || true
    fi

    if [ -d ${S}/int/data/sonidos ]; then
        cp -r ${S}/int/data/sonidos/* ${D}/home/data/sonidos/ || true
    fi

    # 6. Servicio
    install -m 0644 ${WORKDIR}/robot.service ${D}${systemd_system_unitdir}/

    # --- EL FIX CRÍTICO ---
    # Forzamos que TODO en /home le pertenezca a root dentro de la imagen.
    # Esto "limpia" el UID 1000 de tu laptop de los archivos.
    chown -R root:root ${D}/home
    chmod -R 755 ${D}/home
}

# Empaquetado recursivo
FILES:${PN} += " \
    ${libdir}/librobot.so \
    ${bindir}/robot_int_serv \
    ${sysconfdir}/asound.conf \
    ${systemd_system_unitdir}/robot.service \
    /home/web \
    /home/data \
"

FILES_SOLIBSDEV = ""
INSANE_SKIP:${PN} += "dev-so ldflags"
SECTION = "utils"

# EXTRA_OECMAKE += "-DGPIO_CHIP_NAME=\"gpiochip0\""
