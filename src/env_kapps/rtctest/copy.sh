#!/bin/bash

echo "Copiando rtctest.elf para /media/pdsilva/0042-0493/rtctest.elf"
echo "O nome tem que ser rtctest.elf"

cp rtctest.elf /media/pdsilva/0042-0493/rtctest.elf
ls -ltr /media/pdsilva/0042-0493/
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/0042-0493
echo "Unidade desmontada pode retirar o disco..."
