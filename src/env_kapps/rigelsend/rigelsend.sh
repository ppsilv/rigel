#!/usr/bin/env bash
# rigelsend -- envia um arquivo para o rxfile do Rigel, de qualquer pasta do PC.
#
#   rigelsend ARQUIVO [NOME_NO_RIGEL]
#
# Instalacao (uma vez):
#   1) ponha este script e o sendfile.py na mesma pasta (ex.: ~/Projects/rigel/rigel/tools)
#   2) chmod +x rigelsend
#   3) crie um atalho numa pasta do PATH:   ln -s ~/Projects/rigel/rigel/tools/rigelsend ~/bin/rigelsend
#      (se ~/bin nao existe: mkdir -p ~/bin  e coloque  export PATH="$HOME/bin:$PATH"  no ~/.bashrc)
#
# Variaveis opcionais (para mudar sem editar o script):
#   RIGEL_IP=192.168.1.50   RIGEL_PORT=7001   RIGEL_SENDFILE=/caminho/do/sendfile.py
# Opcoes extras do sendfile.py (-l, --crc) tambem funcionam:  rigelsend -l ARQUIVO
set -eu

IP="${RIGEL_IP:-192.168.1.50}"
PORT="${RIGEL_PORT:-7001}"

# pasta real deste script, mesmo quando chamado por um link simbolico
SELF="$(readlink -f "${BASH_SOURCE[0]}")"
HERE="$(dirname "$SELF")"
SENDFILE="${RIGEL_SENDFILE:-$HERE/sendfile.py}"

if [ "$#" -lt 1 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "uso: rigelsend [-l] [--crc] ARQUIVO [NOME_NO_RIGEL]"
    echo "     envia para $IP:$PORT (rxfile do Rigel); nome no Rigel: ate 12 caracteres (8.3)"
    exit 1
fi

if [ ! -f "$SENDFILE" ]; then
    echo "rigelsend: nao achei o sendfile.py em $SENDFILE" >&2
    echo "           ponha-o na mesma pasta do script, ou defina RIGEL_SENDFILE" >&2
    exit 1
fi

# separa as opcoes (-l, --crc) dos argumentos: o sendfile.py quer opcoes antes do IP
OPTS=()
while [ "$#" -gt 0 ]; do
    case "$1" in
        -l|--crc) OPTS+=("$1"); shift ;;
        *) break ;;
    esac
done

if [ "$#" -lt 1 ]; then
    echo "rigelsend: falta o ARQUIVO" >&2
    exit 1
fi
if [ ! -f "$1" ]; then
    echo "rigelsend: arquivo '$1' nao existe" >&2
    exit 1
fi

# $1 = arquivo, $2 (opcional) = nome no Rigel. O caminho relativo vale a partir da pasta atual.
exec python3 "$SENDFILE" ${OPTS[@]+"${OPTS[@]}"} "$IP" "$PORT" "$@"
