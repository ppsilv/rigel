#ifndef TELNET_PROTO_H
#define TELNET_PROTO_H
/*
 * telnet_proto.h -- o minimo do protocolo telnet (RFC 854) para o telnetd do Rigel.
 * So' funcoes puras sobre buffers (sem E/S), para testar no PC.
 *
 * Entrada (cliente -> shell): tira as sequencias IAC, entrega 0xFF literal (IAC IAC),
 *   converte "CR LF" e "CR NUL" em um unico '\r' (o Enter do /dev/vcon).
 * Saida (shell -> cliente): '\n' vira "\r\n", '\r' e' descartado (como o /dev/vcon faz),
 *   0xFF vira "IAC IAC".
 * Negociacao: o servidor anuncia WILL ECHO + WILL SGA + DO SGA, o que poe os clientes
 *   comuns em modo caractere (cada tecla vai na hora; o eco e' do servidor).
 *   As respostas do cliente (DO/DONT/WILL/WONT) sao lidas e ignoradas.
 */
#include <stdint.h>
#include <stddef.h>

#define TN_SE    240
#define TN_SB    250
#define TN_WILL  251
#define TN_WONT  252
#define TN_DO    253
#define TN_DONT  254
#define TN_IAC   255

#define TN_OPT_ECHO  1
#define TN_OPT_SGA   3

enum { TN_ST_DATA = 0, TN_ST_IAC, TN_ST_OPT, TN_ST_SB, TN_ST_SB_IAC };

typedef struct {
    uint8_t state;
    uint8_t after_cr;       /* o byte anterior foi CR: engole o LF ou NUL seguinte */
} tn_in_t;

/* Sequencia enviada ao cliente quando ele conecta */
static const uint8_t tn_greeting[] = {
    TN_IAC, TN_WILL, TN_OPT_ECHO,
    TN_IAC, TN_WILL, TN_OPT_SGA,
    TN_IAC, TN_DO,   TN_OPT_SGA
};

static inline void tn_in_init(tn_in_t *s)
{
    s->state = TN_ST_DATA;
    s->after_cr = 0;
}

/* Filtra 'n' bytes vindos da rede. 'dst' precisa caber 'n' bytes. Devolve quantos escreveu.
 * Pode ser chamada com pedacos de qualquer tamanho (guarda o estado entre chamadas). */
static inline size_t tn_filter_in(tn_in_t *s, const uint8_t *src, size_t n, uint8_t *dst)
{
    size_t o = 0, i;

    for (i = 0; i < n; i++) {
        uint8_t c = src[i];

        switch (s->state) {
        case TN_ST_DATA:
            if (s->after_cr) {
                s->after_cr = 0;
                if (c == '\n' || c == 0)
                    break;                          /* CR LF / CR NUL: o CR ja' foi entregue */
            }
            if (c == TN_IAC) {
                s->state = TN_ST_IAC;
            } else {
                dst[o++] = c;
                if (c == '\r')
                    s->after_cr = 1;
            }
            break;
        case TN_ST_IAC:
            if (c == TN_IAC) {                      /* IAC IAC = o byte 0xFF */
                dst[o++] = TN_IAC;
                s->state = TN_ST_DATA;
            } else if (c >= TN_WILL && c <= TN_DONT) {
                s->state = TN_ST_OPT;               /* falta o byte da opcao */
            } else if (c == TN_SB) {
                s->state = TN_ST_SB;
            } else {
                s->state = TN_ST_DATA;              /* NOP, AYT, IP, etc.: ignora */
            }
            break;
        case TN_ST_OPT:
            s->state = TN_ST_DATA;
            break;
        case TN_ST_SB:                              /* subnegociacao: pula ate' IAC SE */
            if (c == TN_IAC)
                s->state = TN_ST_SB_IAC;
            break;
        case TN_ST_SB_IAC:
            s->state = (c == TN_SE) ? TN_ST_DATA : TN_ST_SB;
            break;
        }
    }
    return o;
}

/* Codifica 'n' bytes da tela do shell para a rede. 'dst' precisa caber 2*n bytes. */
static inline size_t tn_encode_out(const uint8_t *src, size_t n, uint8_t *dst)
{
    size_t o = 0, i;

    for (i = 0; i < n; i++) {
        uint8_t c = src[i];

        if (c == '\r')
            continue;
        if (c == '\n') {
            dst[o++] = '\r';
            dst[o++] = '\n';
        } else if (c == TN_IAC) {
            dst[o++] = TN_IAC;
            dst[o++] = TN_IAC;
        } else {
            dst[o++] = c;
        }
    }
    return o;
}

#endif
