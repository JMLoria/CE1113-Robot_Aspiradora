SUMMARY = "Proyecto Integrador - Robot Aspiradora"
SECTION = "apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Dependencias de compilación (SDK)
DEPENDS = "boost libgpiod alsa-lib"

# Dependencias de ejecución (Runtime)
RDEPENDS:${PN} = "mpg123 alsa-utils alsa-plugins"

# Archivos fuente (Copiados de tu carpeta src)
SRC_URI = "file://librobot \
           file://int"

S = "${WORKDIR}"

inherit cmake

# Configuramos la compilación de ambos componentes
do_compile() {
    # 1. Compilar la librería de hardware (OS)
    cd ${S}/librobot
    mkdir -p build && cd build
    cmake ..
    make

    # 2. Compilar el servidor web (INT)
    cd ${S}/int
    mkdir -p build && cd build
    cmake ..
    make
}

# Instalación en el sistema de archivos de la Raspberry
do_install() {
    # Crear carpetas de destino
    install -d ${D}${libdir}
    install -d ${D}${bindir}
    install -d ${D}/home/web
    install -d ${D}${datadir}/robot/sounds

    # 1. Instalar la librería .so
    install -m 0755 ${S}/librobot/build/librobot.so ${D}${libdir}

    # 2. Instalar el binario del servidor
    install -m 0755 ${S}/int/build/robot_int_serv ${D}${bindir}

    # 3. Instalar la interfaz web
    install -m 0644 ${S}/int/web/index.html ${D}/home/web
    install -m 0644 ${S}/int/web/app.js ${D}/home/web
    install -m 0644 ${S}/int/web/style.css ${D}/home/web

    # 4. Instalar sonidos (si tienes archivos .mp3)
    # install -m 0644 ${S}/int/sounds/*.mp3 ${D}${datadir}/robot/sounds
}

# Aseguramos que los archivos de la web se incluyan en el paquete final
FILES:${PN} += "/home/web/* ${datadir}/robot/sounds/*"