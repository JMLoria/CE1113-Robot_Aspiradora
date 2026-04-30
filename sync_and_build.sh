#!/bin/bash
FILES=~/Embebidos/Proyecto_1/CE1113-Robot_Aspiradora/src/os/yocto/meta-robot/recipes-robot/robot/files

echo "Sincronizando código fuente..."
rm -rf $FILES/int $FILES/librobot
cp -r ~/Embebidos/Proyecto_1/CE1113-Robot_Aspiradora/src/int $FILES/int
cp -r ~/Embebidos/Proyecto_1/CE1113-Robot_Aspiradora/src/os/librobot $FILES/librobot

