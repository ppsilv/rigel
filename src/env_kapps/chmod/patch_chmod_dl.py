#!/usr/bin/env python3
"""patch_chmod_dl.py -- deixa o chmod gravar os bits 6 e 7 (D e L) do atributo.

Uso:   python3 patch_chmod_dl.py PASTA_DO_KERNEL        (ex.: ~/project/rigel/rigel/src/kernel)

Faz duas mudancas (e guarda copia .bak antes de mexer; pode rodar de novo sem estragar):
  1) sys_fsfile.c   sys_chmod: a mascara passa a incluir 0xC0
  2) drivers/fatfs/ff.c   f_chmod: "mask &= AM_RDO | AM_HID | AM_SYS | AM_ARC;" passa a incluir 0xC0
Se alguma linha nao for encontrada o script diz qual; nesse caso me mande a funcao.
"""
import os, re, shutil, sys

def patch(path, pattern, repl, ja_feito, nome):
    if not os.path.isfile(path):
        print("  %s: arquivo nao encontrado (%s)" % (nome, path)); return False
    s = open(path, encoding="utf-8", errors="surrogateescape").read()
    if re.search(ja_feito, s):
        print("  %s: ja esta com 0xC0 (nada a fazer)" % nome); return True
    novo, n = re.subn(pattern, repl, s, count=1)
    if n != 1:
        print("  %s: NAO achei a linha esperada; nada foi alterado" % nome); return False
    shutil.copy2(path, path + ".bak")
    open(path, "w", encoding="utf-8", errors="surrogateescape").write(novo)
    print("  %s: alterado (copia em %s.bak)" % (nome, os.path.basename(path)))
    return True

if len(sys.argv) != 2:
    sys.exit(__doc__)
k = os.path.expanduser(sys.argv[1])
ok = True
print("sys_chmod:")
ok &= patch(os.path.join(k, "sys_fsfile.c"),
            r"(const\s+BYTE\s+all\s*=\s*AM_RDO\s*\|\s*AM_HID\s*\|\s*AM_SYS\s*\|\s*AM_ARC)\s*;",
            r"\1 | 0xC0;",
            r"const\s+BYTE\s+all\s*=[^;]*0xC0", "sys_fsfile.c")
print("f_chmod:")
ok &= patch(os.path.join(k, "drivers", "fatfs", "ff.c"),
            r"(mask\s*&=\s*AM_RDO\s*\|\s*AM_HID\s*\|\s*AM_SYS\s*\|\s*AM_ARC)\s*;",
            r"\1 | 0xC0;",
            r"mask\s*&=[^;]*0xC0", "ff.c")
sys.exit(0 if ok else 1)
