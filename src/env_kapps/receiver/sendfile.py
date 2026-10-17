#!/usr/bin/env python3
"""Envia um arquivo para o rxfile do Rigel.

  python3 sendfile.py [-l] [--crc] IP PORTA ARQUIVO [NOME_NO_RIGEL]

  -l      campos de 32 bits em little-endian (use junto com "rxfile -l")
  --crc   preenche o campo crc32 (padrao: 0 = o Rigel nao confere)
  NOME_NO_RIGEL  ate 12 caracteres (8.3); padrao: nome do arquivo em maiusculas
"""
import os, socket, struct, sys, zlib

args = sys.argv[1:]
little = "-l" in args
use_crc = "--crc" in args
args = [a for a in args if a not in ("-l", "--crc")]
if len(args) < 3:
    sys.exit(__doc__)
ip, port, path = args[0], int(args[1]), args[2]
name = (args[3] if len(args) > 3 else os.path.basename(path).upper())
if len(name) > 12:
    sys.exit("nome '%s' tem mais de 12 caracteres (8.3)" % name)

data = open(path, "rb").read()
crc = zlib.crc32(data) & 0xFFFFFFFF if use_crc else 0
fmt = "<II13s" if little else ">II13s"
hdr = struct.pack(fmt, len(data), crc, name.encode("ascii"))
assert len(hdr) == 21

with socket.create_connection((ip, port), timeout=15) as s:
    s.sendall(hdr)
    s.sendall(data)
    s.shutdown(socket.SHUT_WR)
    try:
        s.recv(1)           # espera o Rigel fechar (so' termina depois de gravar)
    except OSError:
        pass
print("enviado %s (%d bytes) como %s" % (path, len(data), name))
