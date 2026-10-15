#!/bin/bash

echo "Copiando rtcpoke.elf para /media/pdsilva/0042-0493/rtcpoke.elf"
echo "O nome tem que ser rtcpoke.elf"

cp rtcpoke.elf /media/pdsilva/0042-0493/rtcpoke.elf
ls -ltr /media/pdsilva/0042-0493/
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/0042-0493
echo "Unidade desmontada pode retirar o disco..."
