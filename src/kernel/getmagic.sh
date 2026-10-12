#!/bin/bash

m68k-elf-objdump -s -j .text --start-address=0x82000 --stop-address=0x8200C kernel.elf
