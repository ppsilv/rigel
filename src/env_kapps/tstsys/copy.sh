#!/bin/bash

echo "Copiando tstsys.elf para /media/pdsilva/0042-0493/."
cp tstsys.elf /media/pdsilva/0042-0493/tstraw.elf
ls -ltr /media/pdsilva/0042-0493
echo "Umounting disk..."
sudo umount /media/pdsilva/0042-0493