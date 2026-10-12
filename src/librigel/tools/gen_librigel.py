#!/usr/bin/env python3
"""
gen_librigel.py -- gera os stubs de syscall e os headers da librigel a partir do syscall.h do kernel.

Uso:  python3 tools/gen_librigel.py /caminho/do/kernel/include/sys/syscall.h [new|old]

O 2o argumento escolhe QUAL familia de numeros os stubs open/close/read/write/lseek/fsync/ioctl usam.
Tem que ser a que o trap0.c do kernel trata com "case":
  new (padrao): SYS_OPEN SYS_CLOSE SYS_READ SYS_WRITE SYS_LSEEK SYS_SINC SYS_IOCTL
  old         : SYS_FOPEN SYS_FCLOSE SYS_FREAD SYS_FWRITE SYS_FLSEEK SYS_FSYNC SYS_FIOCTL

Gera:
  libsrc/syscalls_gen.S        um stub (numero em D0 + trap #0 + rts) para cada syscall
  include/rigel/<grupo>.h      prototipos agrupados por servico
  include/rigel/rigel.h        inclui todos

Para mudar o prefixo dos wrappers, edite PREFIX abaixo.
NAO edite os arquivos gerados: mude a TABELA aqui e rode de novo.
"""
import re, sys, os

PREFIX = "rigel_"
TRAP = 0          # TRAP #0

# (nome em syscall.h, grupo, nome do wrapper, argumentos C)
# grupo None  = glue da newlib (nome normal, sem prototipo: quem declara e' a newlib)
# argumentos None = sem header
TABLE = [
  # ---- glue da newlib (nomes que a libc chama) ----
  ("EXIT",   None, "_exit",  None), ("OPEN",  None, "open",  None), ("CLOSE", None, "close", None),
  ("READ",   None, "read",   None), ("WRITE", None, "write", None), ("LSEEK", None, "lseek", None),
  ("GETPID", None, "getpid", None), ("FUNLINK", None, "unlink", None),
  ("FRENAME", None, "rename", None), ("FMKDIR", None, "mkdir", None),
  # ---- processo ----
  ("SPAWN",  "proc", "spawn",  "char *path"),
  ("SLEEP",  "proc", "sleep",  "long ticks"),
  ("YIELD",  "proc", "yield",  "void"),
  ("SETRAMVECTOR", "proc", "setramvector", "long stub_addr, long handler_addr"),
  # ---- arquivo / vfs ----
  ("SINC",   "file", "fsync",  "long fd"),
  ("IOCTL",  "file", "ioctl",  "long fd, long cmd, void *arg"),
  ("DUP",    "file", "dup",    "long fd_old, long fd_new"),
  ("CHMOD",  "file", "chmod",  "char *path, long attrs"),
  ("FTELL",  "file", "tell",   "long fd"),
  ("FSIZE",  "file", "size",   "long fd"),
  ("FEOF",   "file", "eof",    "long fd"),
  ("FSTAT",  "file", "stat",   "char *path, void *stat_buf"),
  ("FOPENDIR",  "file", "opendir",  "char *path, void *dir"),
  ("FREADDIR",  "file", "readdir",  "void *dir, void *file_info"),
  ("FMOUNT",    "file", "mount",    "char *path"),
  ("FCLOSEDIR", "file", "closedir", "void *dir"),
  ("FUNMOUNT",  "file", "unmount",  "char *path"),
  ("FFINDFIRST","file", "findfirst","void *dir, void *file_info, char *path, char *pattern"),
  ("FFINDNEXT", "file", "findnext", "void *dir, void *file_info"),
  # ---- video ----
  ("VGAPUTCHAR", "vga", "vga_putchar", "long c"),
  ("GOTOXY",     "vga", "gotoxy",      "long x, long y"),
  ("GOHOME",     "vga", "gohome",      "void"),
  ("SETTEXTCOLOR","vga","settextcolor","long color"),
  ("CLRSCR",     "vga", "clrscr",      "void"),
  ("DRAWVLINE",  "vga", "drawvline",   "long x, long y, long h, long color"),
  ("DRAWHLINE",  "vga", "drawhline",   "long x, long y, long w, long color"),
  ("DRAWLINE",   "vga", "drawline",    "long x0, long y0, long x1, long y1, long color"),
  ("DRAWCIRCLE", "vga", "drawcircle",  "long x0, long y0, long r, long color"),
  ("DRAWCIRCLEHELPER","vga","drawcirclehelper","long x0, long y0, long r, long cornername, long color"),
  ("FILLCIRCLE", "vga", "fillcircle",  "long x0, long y0, long r, long color"),
  ("FILLCIRCLEHELPER","vga","fillcirclehelper","long x0, long y0, long r, long cornername, long delta, long color"),
  ("DRAWROUNDRECT","vga","drawroundrect","long x, long y, long w, long h, long r, long color"),
  ("FILLROUNDRECT","vga","fillroundrect","long x, long y, long w, long h, long r, long color"),
  ("FILLRECT",   "vga", "fillrect",    "long x, long y, long w, long h, long color"),
  # ---- relogio ----
  ("READTIME",  "rtc", "readtime",  "void *tm_out"),
  ("WRITETIME", "rtc", "writetime", "void *tm_in"),
  ("READRTC",   "rtc", "readrtc",   "void *rtc"),      # ASSINATURA SUPOSTA: confira
  ("WRITERTC",  "rtc", "writertc",  "void *rtc"),      # ASSINATURA SUPOSTA: confira
  # ---- eeprom ----
  ("EEPROMREADBYTE",  "eeprom", "eeprom_readbyte",  "long addr"),
  ("EEPROMWRITEBYTE", "eeprom", "eeprom_writebyte", "long addr, long value"),
  # ---- ring buffer ----
  ("RINGBUFFERREAD",  "ringbuf", "ringbuffer_read",  "void *rb"),
  ("RINGBUFFERWRITE", "ringbuf", "ringbuffer_write", "void *rb, long value"),
  # ---- duart ----
  ("DUARTACFG",  "duart", "duarta_cfg",   "long baud, long params"),
  ("DUARTAREAD", "duart", "duarta_read",  "void"),
  ("DUARTAWRITE","duart", "duarta_write", "long value"),
  ("DUARTBCFG",  "duart", "duartb_cfg",   "long baud, long params"),
  ("DUARTBREAD", "duart", "duartb_read",  "void"),
  ("DUARTBWRITE","duart", "duartb_write", "long value"),
  # ---- ppi ----
  ("PPIACFG",       "ppi", "ppia_cfg",         "long mode"),
  ("PPIPORTAREAD",  "ppi", "ppi_porta_read",   "void"),
  ("PPIPORTAWRITE", "ppi", "ppi_porta_write",  "long value"),
  ("PPIBCFG",       "ppi", "ppib_cfg",         "long mode"),
  ("PPIPORTBREAD",  "ppi", "ppi_portb_read",   "void"),
  ("PPIPORTBWRITE", "ppi", "ppi_portb_write",  "long value"),
  ("PPICCFG",       "ppi", "ppic_cfg",         "long mode"),
  ("PPIPORTCREAD",  "ppi", "ppi_portc_read",   "void"),
  ("PPIPORTCWRITE", "ppi", "ppi_portc_write",  "long value"),
  # ---- rede ----
  ("CONNECT",    "net", "connect",    "long sockfd, void *addr, long addrlen"),
  ("BIND",       "net", "bind",       "long sockfd, void *addr, long addrlen"),
  ("LISTEN",     "net", "listen",     "long sockfd, long backlog"),
  ("ACCEPT",     "net", "accept",     "long sockfd, void *addr, long *addrlen"),
  ("SETSOCKOPT", "net", "setsockopt", "long sockfd, long level, long optname, void *optval, long optlen"),
]

# As duas familias de numeros para as mesmas operacoes (nova -> antiga)
NEW2OLD = {"OPEN":"FOPEN","CLOSE":"FCLOSE","READ":"FREAD","WRITE":"FWRITE",
           "LSEEK":"FLSEEK","SINC":"FSYNC","IOCTL":"FIOCTL"}

def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    family = sys.argv[2] if len(sys.argv) == 3 else "new"
    if family not in ("new", "old"):
        sys.exit("2o argumento: new ou old")
    if family == "old":
        table = [(NEW2OLD.get(t[0], t[0]),) + t[1:] for t in TABLE]
        SKIP = set(NEW2OLD.keys())            # a familia nova nao gera wrapper
    else:
        table = TABLE
        SKIP = set(NEW2OLD.values())          # a familia antiga nao gera wrapper
    nums = {}
    for line in open(sys.argv[1], encoding="utf-8", errors="replace"):
        m = re.match(r"\s*#\s*define\s+SYS_(\w+)\s+(0[xX][0-9a-fA-F]+|\d+)", line)
        if m:
            nums[m.group(1)] = int(m.group(2), 0)

    # --- verificacoes ---
    byval = {}
    for n, v in nums.items():
        byval.setdefault(v, []).append(n)
    dups = {v: ns for v, ns in byval.items() if len(ns) > 1}
    if dups:
        for v, ns in dups.items():
            print(f"ERRO: mesmo numero 0x{v:04X} para {', '.join('SYS_'+n for n in ns)}")
        sys.exit(1)
    known = {t[0] for t in table} | SKIP
    unknown = sorted(set(nums) - known)
    missing = sorted(known - set(nums) - SKIP)
    for n in unknown:
        print(f"AVISO: SYS_{n} esta no syscall.h mas nao na TABELA do gerador (sem wrapper)")
    for n in missing:
        print(f"AVISO: {n} esta na TABELA mas nao no syscall.h (ignorado)")

    base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    # --- stubs ---
    out = ["/* GERADO por tools/gen_librigel.py a partir do syscall.h -- NAO EDITE. */",
           "        .macro  SYSCALL name, num",
           "        .global \\name",
           "\\name:",
           "        move.l  #\\num,%d0",
           f"        trap    #{TRAP}",
           "        rts",
           "        .endm",
           "", "        .text"]
    groups = {}
    for sysn, grp, wrap, args in table:
        if sysn not in nums:
            continue
        name = wrap if grp is None else PREFIX + wrap
        out.append(f"        SYSCALL {name:<28} 0x{nums[sysn]:04X}   /* SYS_{sysn} */")
        if grp:
            groups.setdefault(grp, []).append((name, args))
    open(os.path.join(base, "libsrc", "syscalls_gen.S"), "w").write("\n".join(out) + "\n")

    # --- headers ---
    inc = os.path.join(base, "include", "rigel")
    for grp, items in groups.items():
        g = f"RIGEL_{grp.upper()}_H"
        h = [f"/* GERADO por tools/gen_librigel.py -- NAO EDITE. */",
             f"#ifndef {g}", f"#define {g}", "",
             "#ifdef __cplusplus", 'extern "C" {', "#endif", ""]
        for name, args in items:
            h.append(f"long {name}({args});")
        h += ["", "#ifdef __cplusplus", "}", "#endif", "", f"#endif /* {g} */", ""]
        open(os.path.join(inc, grp + ".h"), "w").write("\n".join(h))
    um = ["/* GERADO -- inclui todos os servicos do Rigel. */", "#ifndef RIGEL_H", "#define RIGEL_H"]
    um += [f"#include <rigel/{g}.h>" for g in groups]
    um += ["#endif", ""]
    open(os.path.join(inc, "rigel.h"), "w").write("\n".join(um))

    ngen = sum(1 for t in table if t[0] in nums)
    print(f"ok: familia '{family}'; {ngen} wrappers gerados; {len(SKIP & set(nums))} da outra familia ignorados")

main()
