#!/usr/bin/env python3
"""
patch_netmon.py -- liga o monitor de rede (w5100_netmon.h) no w5100_netif.c.

Uso (na pasta onde esta o w5100_netif.c e onde voce copiou o w5100_netmon.h):
    python3 patch_netmon.py w5100_netif.c

- Idempotente: se ja foi aplicado, nao faz nada.
- Tudo ou nada: se algum trecho nao for encontrado, NAO altera o arquivo e diz qual faltou.
- Guarda w5100_netif.c.bak_netmon antes de alterar.
"""
import re, sys, shutil

if len(sys.argv) != 2:
    print(__doc__); sys.exit(1)
path = sys.argv[1]
src = open(path, encoding="utf-8", errors="surrogateescape").read()

if 'w5100_netmon.h' in src:
    print("ja aplicado: nada a fazer"); sys.exit(0)

def sub1(pat, repl, txt, nome, flags=0):
    m = list(re.finditer(pat, txt, flags))
    if len(m) != 1:
        print("ERRO: trecho '%s' encontrado %d vez(es) (esperado 1). Nada foi alterado." % (nome, len(m)))
        sys.exit(2)
    return txt[:m[0].start()] + repl(m[0]) + txt[m[0].end():]

s = src

# 1) variaveis do monitor (logo apos cfg_mac)
s = sub1(r'static const uint8_t cfg_mac\[6\] = NET_MAC;[^\n]*\n',
         lambda m: m.group(0) +
         "\n/* ---- monitor de rede (w5100_netmon.h) ---- */\n"
         "static volatile uint32_t nm_rx_loops, nm_rx_frames, nm_rx_nomem, nm_tx_frames, nm_tx_errors;\n"
         "static volatile uint8_t  nm_rx_stage, nm_tx_stage;\n",
         s, "cfg_mac")

# 2) low_level_output vira low_level_output_i (o wrapper nm_linkoutput fica no .h)
s = sub1(r'static err_t low_level_output\(struct netif \*netif, struct pbuf \*p\)',
         lambda m: 'static err_t low_level_output_i(struct netif *netif, struct pbuf *p)', s, "low_level_output")

# 3) estagio do tx depois de pegar o lock
s = sub1(r'DBG\("  TX lock ok\\n"\);',
         lambda m: m.group(0) + "\n    nm_tx_stage = 2;", s, "TX lock ok")

# 4) estagio do rx ao ler o quadro
s = sub1(r'\n    rsr = stable16\(S0_RX_RSR\);',
         lambda m: "\n    nm_rx_stage = 3;" + m.group(0), s, "rsr = stable16")

# 5) conta quadros sem memoria
s = sub1(r'if \(p == NULL\)\s*\n\s*ptr \+= len;[^\n]*',
         lambda m: 'if (p == NULL) { nm_rx_nomem++; ptr += len; }          /* sem memoria: pula o quadro */',
         s, "p == NULL ptr += len")

# 6) laco da task rx
s = sub1(r'        while \(\(p = low_level_input\(\)\) != NULL\) \{\n',
         lambda m: ("        nm_rx_loops++;\n"
                    "        for (;;) {\n"
                    "            nm_rx_stage = 1;\n"
                    "            p = low_level_input();\n"
                    "            if (p == NULL) { nm_rx_stage = 0; break; }\n"
                    "            nm_rx_frames++;\n"
                    "            nm_rx_stage = 2;\n"),
         s, "while low_level_input")

# 7) o netif usa o wrapper de envio
s = sub1(r'netif->linkoutput = low_level_output;',
         lambda m: 'netif->linkoutput = nm_linkoutput;', s, "linkoutput")

# 8) inclui o monitor antes da secao netif
s = sub1(r'/\* -{10,} netif \*/',
         lambda m: '#include "w5100_netmon.h"\n\n' + m.group(0), s, "secao netif")

# 9) liga o monitor no start
s = sub1(r'\n    started = 1;',
         lambda m: "\n    nm_start();" + m.group(0), s, "started = 1")

shutil.copy(path, path + ".bak_netmon")
open(path, "w", encoding="utf-8", errors="surrogateescape").write(s)
print("OK: monitor ligado em %s (backup: %s.bak_netmon)" % (path, path))
