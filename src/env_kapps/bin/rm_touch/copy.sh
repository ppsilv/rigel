#!/bin/bash

echo "Copiando rm.elf e touch.elf para /media/pdsilva/0042-0493/"

cp rm.elf /media/pdsilva/0042-0493/bin/rm.elf
cp touch.elf /media/pdsilva/0042-0493/bin/touch.elf
ls -ltr /media/pdsilva/0042-0493/bin
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/0042-0493
echo "Unidade desmontada pode retirar o disco..."
