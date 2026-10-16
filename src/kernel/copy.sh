#!/bin/bash

echo "Copiando kernel.sys para /media/pdsilva/rigel/kernel.bin"
echo "O nome tem que ser kernel.bin"

cp kernel.sys /media/pdsilva/rigel/boot/kernel.bin
ls -ltr /media/pdsilva/rigel/boot
echo "DESMONTANDO O DISCO"
sudo umount /media/pdsilva/rigel
echo "PODE RETIRAR O DISCO DA UNIDADE"