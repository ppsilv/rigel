#!/usr/bin/env python3
"""
patch_netmon2.py -- 2a etapa do monitor de rede (use DEPOIS do patch_netmon.py).

Uso (na pasta do w5100_netif.c, com o w5100_netmon.h NOVO ja copiado por cima do antigo):
    python3 patch_netmon2.py w5100_netif.c

O que faz no w5100_netif.c:
  - stable16(): passa a desistir depois de 100 leituras instaveis (antes podia girar para sempre
    segurando o lock do chip); devolve 0 e conta em nm_unstable;
  - conta quadros com tamanho invalido (nm_rx_garbage);
  - estagios mais finos da task rx (3 a 7) e guarda o TCB dela (nm_rx_tcb) para o monitor mostrar
    o estado da task no alerta.

Idempotente, tudo-ou-nada, guarda w5100_netif.c.bak_netmon2.
"""
import re, sys, shutil

if len(sys.argv) != 2:
    print(__doc__); sys.exit(1)
path = sys.argv[1]
src = open(path, encoding="utf-8", errors="surrogateescape").read()

if 'nm_rx_tcb' in src:
    print("ja aplicado: nada a fazer"); sys.exit(0)
if 'w5100_netmon.h' not in src:
    print("ERRO: rode primeiro o patch_netmon.py. Nada foi alterado."); sys.exit(2)

def sub1(pat, repl, txt, nome):
    m = list(re.finditer(pat, txt))
    if len(m) != 1:
        print("ERRO: trecho '%s' encontrado %d vez(es) (esperado 1). Nada foi alterado." % (nome, len(m)))
        sys.exit(2)
    return txt[:m[0].start()] + repl(m[0]) + txt[m[0].end():]

s = src

# 1) declaracoes antes do stable16
s = sub1(r'static uint16_t stable16\(uint16_t r\)',
         lambda m: ("/* ---- monitor de rede (etapa 2) ---- */\n"
                    "extern struct tcb *current_task;\n"
                    "static volatile uint32_t nm_unstable, nm_rx_garbage;\n"
                    "static void *volatile    nm_rx_tcb;\n\n") + m.group(0),
         s, "stable16")

# 2) stable16 limitado
s = sub1(r'do \{ a = rd16\(r\); b = rd16\(r\); \} while \(a != b\);\s*\n\s*return a;',
         lambda m: ("int n = 0;\n"
                    "    do { a = rd16(r); b = rd16(r); } while (a != b && ++n < 100);\n"
                    "    if (a != b) { nm_unstable++; return 0; }     /* leitura instavel: nao prende o lock do chip */\n"
                    "    return a;"),
         s, "corpo do stable16")

# 3) estagios finos da rx
s = sub1(r'\n    ptr = rd16\(S0_RX_RD\);',
         lambda m: m.group(0) + "\n    nm_rx_stage = 4;", s, "ptr = rd16(S0_RX_RD)")
s = sub1(r'if \(len < 2 \|\| len - 2 > MAX_FRAME \|\| len > rsr\) \{[^\n]*\n',
         lambda m: m.group(0) + "        nm_rx_garbage++;\n", s, "teste de tamanho invalido")
s = sub1(r'    p = pbuf_alloc\(PBUF_RAW, len, PBUF_POOL\);',
         lambda m: "    nm_rx_stage = 5;\n" + m.group(0) + "\n    nm_rx_stage = 6;", s, "pbuf_alloc")
s = sub1(r'    wr16\(S0_RX_RD, ptr\);\s*\n\s*cmd0\(CMD_RECV\);\s*\nout:',
         lambda m: "    nm_rx_stage = 7;\n" + m.group(0), s, "wr16 final + out:")

# 4) guarda o TCB da task rx
s = sub1(r'        nm_rx_loops\+\+;\n',
         lambda m: m.group(0) + "        nm_rx_tcb = current_task;\n", s, "nm_rx_loops++")

shutil.copy(path, path + ".bak_netmon2")
open(path, "w", encoding="utf-8", errors="surrogateescape").write(s)
print("OK: etapa 2 aplicada em %s (backup: %s.bak_netmon2)" % (path, path))
