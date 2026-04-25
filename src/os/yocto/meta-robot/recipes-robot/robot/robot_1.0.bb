SUMMARY = "Proyecto Integrador - Robot Aspiradora"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "boost libgpiod alsa-lib mpg123"
RDEPENDS:${PN} = "mpg123 alsa-utils"

# IMPORTANTE: Incluimos el nuevo CMakeLists.txt maestro
SRC_URI = "file://CMakeLists.txt \
           file://librobot \
           file://int"

S = "${WORKDIR}"

inherit cmake pkgconfig

do_install() {
    install -d ${D}${libdir}
    install -d ${D}${bindir}
    install -d ${D}/home/web

    find ${B} -name "librobot.so" -exec install -m 0755 {} ${D}${libdir}/ \;
    find ${B} -name "robot_int_serv" -exec install -m 0755 {} ${D}${bindir}/ \;

    install -m 0644 ${S}/int/web/index.html ${D}/home/web/
    install -m 0644 ${S}/int/web/app.js     ${D}/home/web/
    install -m 0644 ${S}/int/web/style.css  ${D}/home/web/
}

FILES:${PN} = " \
    ${bindir} \
    ${bindir}/* \
    ${libdir} \
    ${libdir}/* \
    /home/web \
    /home/web/* \
"
FILES:${PN}-dev = ""

# 2. Manejo de librerías .so 
FILES_SOLIBSDEV = ""
INSANE_SKIP:${PN}:append = " dev-so ldflags"


SECTION = "utils"

# Definimos el chip por defecto (Pi 4)
EXTRA_OECMAKE += "-DGPIO_CHIP_NAME=\"gpiochip0\""

# Si la máquina es Raspberry Pi 5, sobrescribimos la bandera
EXTRA_OECMAKE:append:raspberrypi5 = " -DGPIO_CHIP_NAME=\"gpiochip4\""