#!/bin/bash

echo "Copiando nettest.elf para /media/pdsilva/rigel/nettest.elf"
echo "O nome tem que ser nettest.elf"

cp nettest.elf /media/pdsilva/rigel/nettest.elf
ls -ltr /media/pdsilva/rigel
echo "Demonstando a unidade..."
sudo umount /media/pdsilva/rigel
echo "Unidade desmontada pode retirar o disco..."