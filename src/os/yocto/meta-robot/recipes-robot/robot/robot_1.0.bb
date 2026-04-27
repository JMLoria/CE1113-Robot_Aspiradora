SUMMARY = "Proyecto Integrador - Robot Aspiradora"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Incrementamos PR (Package Revision) cada vez que cambies el código físicamente
# Esto obliga a Yocto a reconstruir el paquete y la imagen
PR = "r1"

DEPENDS = "boost libgpiod alsa-lib mpg123"
RDEPENDS:${PN} = "mpg123 alsa-utils"

SRC_URI = "file://CMakeLists.txt \
           file://librobot \
           file://int"

S = "${WORKDIR}"

inherit cmake pkgconfig systemd

do_install() {
    install -d ${D}${libdir}
    install -d ${D}${bindir}
    install -d ${D}/home/web
    install -d ${D}/usr/share/robot/sounds
    install -d ${D}/usr/share/robot/musics

    # 1. Ruta la librería 
    install -m 0755 ${B}/librobot/librobot.so ${D}${libdir}/

    # 2. Ruta para el servidor 
    install -m 0755 ${B}/int/robot_int_serv ${D}${bindir}/

    # 3. Interfaz web 
    install -m 0644 ${S}/int/web/index.html ${D}/home/web/
    install -m 0644 ${S}/int/web/app.js     ${D}/home/web/
    install -m 0644 ${S}/int/web/style.css  ${D}/home/web/
}

SRC_URI += "file://robot.service"


SYSTEMD_SERVICE:${PN} = "robot.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/robot.service ${D}${systemd_unitdir}/system/
}

FILES:${PN} = " \
    ${bindir} \
    ${bindir}/* \
    ${libdir} \
    ${libdir}/* \
    /home/web \
    /home/web/* \
"
FILES:${PN} += "${systemd_unitdir}/system/robot.service"
FILES:${PN} += "/usr/share/robot /usr/share/robot/*"
FILES:${PN}-dev = ""

# 2. Manejo de librerías .so 
FILES_SOLIBSDEV = ""
INSANE_SKIP:${PN}:append = " dev-so ldflags"
SECTION = "utils"

# Definimos el chip por defecto (Pi 4)
EXTRA_OECMAKE += "-DGPIO_CHIP_NAME=gpiochip0"

# Si la máquina es Raspberry Pi 5, sobrescribimos la bandera
EXTRA_OECMAKE:append:raspberrypi5 = " -DGPIO_CHIP_NAME=gpiochip4"