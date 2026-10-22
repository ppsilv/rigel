#!/bin/bash

echo "Copiando cmds para /media/pdsilva/0042-0493/"


cp *.elf /media/pdsilva/0042-0493/*.elf
ls -ltr /media/pdsilva/0042-0493/
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/0042-0493
echo "Unidade desmontada pode retirar o disco..."
