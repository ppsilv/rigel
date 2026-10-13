#!/bin/bash

echo "Copiando kernel.sys para /media/pdsilva/0042-0493/kernel.bin"
echo "O nome tem que ser kernel.bin"

cp kernel.sys /media/pdsilva/0042-0493/boot/kernel.bin
ls -ltr /media/pdsilva/0042-0493/boot
echo "DESMONTANDO O DISCO"
sudo umount /media/pdsilva/0042-0493
echo "PODE RETIRAR O DISCO DA UNIDADE"