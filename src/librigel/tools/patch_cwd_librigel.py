#!/usr/bin/env python3
"""
patch_cwd_librigel.py -- garante CHDIR e GETCWD na TABLE do SEU tools/gen_librigel.py.

Uso:   python3 patch_cwd_librigel.py /caminho/para/src/librigel

Gera (grupo "file", header include/rigel/file.h):
    long rigel_chdir(char *path);
    long rigel_getcwd(char *buf, long size);

Acrescenta as duas entradas no comeco da TABLE (ou troca, se ja existirem). Nao mexe no resto.
Pode rodar mais de uma vez. Depois: make da librigel (com SYSCALL_H=.../ksyscall.h VFS_H=.../vfs.h).
"""
import os, re, sys
if len(sys.argv) != 2:
    sys.exit(__doc__)
p = os.path.join(sys.argv[1], "tools", "gen_librigel.py")
if not os.path.exists(p):
    sys.exit(f"ERRO: nao achei {p}")
s = open(p).read()

novas = {
    "CHDIR":  '("CHDIR",  "file", "chdir",  "char *path")',
    "GETCWD": '("GETCWD", "file", "getcwd", "char *buf, long size")',
}
feito = []
for nome, tupla in novas.items():
    pat = re.compile(r'\(\s*"%s"\s*,[^()]*\)' % nome)
    if pat.search(s):
        if tupla not in s:
            s = pat.sub(tupla, s, count=1)
            feito.append(nome + " (trocada)")
    else:
        m = re.search(r'^TABLE\s*=\s*\[[ \t]*\n', s, re.M)
        if not m:
            sys.exit("ERRO: nao achei 'TABLE = [' no gen_librigel.py (me mande a sua TABLE)")
        s = s[:m.end()] + "  " + tupla + ",\n" + s[m.end():]
        feito.append(nome + " (acrescentada)")
open(p, "w").write(s)
print("ok:", ", ".join(feito) if feito else "ja estava tudo certo.")
print("Agora rode o make da librigel.")
