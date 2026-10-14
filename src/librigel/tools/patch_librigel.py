#!/usr/bin/env python3
"""
patch_librigel.py -- ajusta a SUA librigel (sem apagar o que voce acrescentou) para as syscalls de arquivo.

Uso:   python3 patch_librigel.py /caminho/para/src/librigel

Faz (e pode rodar mais de uma vez, ele nao duplica nada):
  1. tools/gen_librigel.py : troca as linhas FFINDFIRST/FFINDNEXT pelas novas e acrescenta
                             rigel_ff_open/close/read/write/sync/lseek/ioctl (familia SYS_F*);
                             faz o rigel.h incluir <rigel/fsflags.h>
  2. Makefile              : copia kernel/include/sys/rigel_fsflags.h para include/rigel/fsflags.h
"""
import os, re, sys

if len(sys.argv) != 2:
    sys.exit(__doc__)
root = sys.argv[1]
gen = os.path.join(root, "tools", "gen_librigel.py")
mk  = os.path.join(root, "Makefile")
for p in (gen, mk):
    if not os.path.exists(p):
        sys.exit(f"ERRO: nao achei {p}")

# ---------------------------------------------------------------- gen_librigel.py
s = open(gen).read()
n_changes = 0

# 1) findfirst / findnext
pat_ff = re.compile(r'\("FFINDFIRST",\s*"file",\s*"findfirst",\s*"[^"]*"\)')
pat_fn = re.compile(r'\("FFINDNEXT",\s*"file",\s*"findnext",\s*"[^"]*"\)')
new_ff = '("FFINDFIRST","file", "findfirst","char *path, char *pattern, void *ent")'
new_fn = '("FFINDNEXT", "file", "findnext", "long dir, void *ent")'
if not pat_ff.search(s) or not pat_fn.search(s):
    sys.exit("ERRO: nao achei as linhas FFINDFIRST/FFINDNEXT na TABLE de gen_librigel.py")
if new_ff not in s:
    s = pat_ff.sub(new_ff, s); n_changes += 1
if new_fn not in s:
    s = pat_fn.sub(new_fn, s); n_changes += 1

# 2) familia SYS_F* (7 wrappers com prefixo ff_)
if '"ff_open"' not in s:
    block = '''  # ---- arquivos FAT (familia SYS_F*; handles do FatFS, NAO sao fds do VFS) ----
  ("FOPEN",  "file", "ff_open",  "char *path, long flags"),
  ("FCLOSE", "file", "ff_close", "long h"),
  ("FREAD",  "file", "ff_read",  "long h, void *buf, long size"),
  ("FWRITE", "file", "ff_write", "long h, void *buf, long size"),
  ("FSYNC",  "file", "ff_sync",  "long h"),
  ("FLSEEK", "file", "ff_lseek", "long h, long offset, long whence"),
  ("FIOCTL", "file", "ff_ioctl", "long h, long cmd, void *arg"),
'''
    marker = "  # ---- video ----"
    if marker not in s:
        sys.exit("ERRO: nao achei '# ---- video ----' na TABLE")
    s = s.replace(marker, block + marker, 1); n_changes += 1

# 3) rigel.h inclui fsflags.h
if "fsflags.h" not in s:
    anchor = '    um += ["#endif", ""]'
    if anchor not in s:
        sys.exit("ERRO: nao achei o final do rigel.h em main()")
    s = s.replace(anchor,
        '    if os.path.exists(os.path.join(inc, "fsflags.h")):\n'
        '        um += ["#include <rigel/fsflags.h>"]\n' + anchor, 1); n_changes += 1
open(gen, "w").write(s)

# ---------------------------------------------------------------- Makefile
m = open(mk).read()
if "FSFLAGS_H" not in m:
    # variavel
    if "DIRENT_H" in m:
        m = re.sub(r'(DIRENT_H[^\n]*\n)',
                   r'\1FSFLAGS_H ?= $(RIGELDIR)/src/kernel/include/sys/rigel_fsflags.h\n', m, count=1)
    else:
        m = re.sub(r'(\nFAMILY[^\n]*\n)',
                   r'\nFSFLAGS_H ?= $(RIGELDIR)/src/kernel/include/sys/rigel_fsflags.h\1', m, count=1)
    # all:
    m, k = re.subn(r'^all:\s*', 'all: include/rigel/fsflags.h ', m, count=1, flags=re.M)
    if k == 0:
        sys.exit("ERRO: nao achei a linha 'all:' no Makefile")
    # regra
    m = m.replace("\nFORCE:", "\n# fsflags.h vem do kernel (arquivo mestre)\ninclude/rigel/fsflags.h: FORCE\n\tcp $(FSFLAGS_H) $@\n\nFORCE:", 1)
    if "include/rigel/fsflags.h: FORCE" not in m:
        sys.exit("ERRO: nao achei 'FORCE:' no Makefile")
    n_changes += 1
open(mk, "w").write(m)

print(f"ok: {n_changes} alteracao(oes) feita(s). Agora rode o make da librigel.")
