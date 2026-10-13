#!/bin/bash

echo "Copiando shell.elf para /media/pdsilva/0042-0493/shell.elf"
echo "O nome tem que ser shell.elf"

cp shell.elf /media/pdsilva/0042-0493/shell.elf
ls -ltr /media/pdsilva/0042-0493/
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/0042-0493
echo "Unidade desmontada pode retirar o disco..."
