#!/bin/bash

echo "Copiando shell.elf para /media/pdsilva/0042-0493/shell.elf"
echo "O nome tem que ser shell.elf"

cp shell.elf /media/pdsilva/rigel/shell.elf
ls -ltr /media/pdsilva/rigel
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/rigel
echo "Unidade desmontada pode retirar o disco..."
