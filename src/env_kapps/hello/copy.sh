#!/bin/bash

echo "Copiando hello.elf para /media/pdsilva/0042-0493/hello.elf"
echo "O nome tem que ser hello.elf"

cp hello.elf /media/pdsilva/0042-0493/hello.elf
ls -ltr /media/pdsilva/0042-0493/
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/0042-0493
echo "Unidade desmontada pode retirar o disco..."
