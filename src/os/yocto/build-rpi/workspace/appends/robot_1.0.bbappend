FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
FILESPATH:prepend := "/home/roy/Embebidos/Proyecto_1/CE1113-Robot_Aspiradora/src/os/yocto/build-rpi/workspace/sources/robot/oe-local-files:"
# srctreebase: /home/roy/Embebidos/Proyecto_1/CE1113-Robot_Aspiradora/src/os/yocto/build-rpi/workspace/sources/robot

inherit externalsrc
# NOTE: We use pn- overrides here to avoid affecting multiple variants in the case where the recipe uses BBCLASSEXTEND
EXTERNALSRC:pn-robot = "/home/roy/Embebidos/Proyecto_1/CE1113-Robot_Aspiradora/src/os/yocto/build-rpi/workspace/sources/robot"

# initial_rev .: a25ac8304ff4eb21f4d5c7b6d963756d829d15ca
# commit .: f1c36d454054781e09d2afd2599f60b0b57155fe
