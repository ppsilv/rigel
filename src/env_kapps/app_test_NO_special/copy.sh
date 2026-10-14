#!/bin/bash

echo "Copiando ftest.elf para /media/pdsilva/0042-0493/ftest.elf"
echo "O nome tem que ser ftest.elf"

cp ftest.elf /media/pdsilva/0042-0493/ftest.elf
ls -ltr /media/pdsilva/0042-0493/
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/0042-0493
echo "Unidade desmontada pode retirar o disco..."
