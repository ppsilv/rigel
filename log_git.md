## 2026/10/12 kernel trap #0 funcionando corretamente
                Aplicação tsttrap.c funcionou completamente.
                vet32 (end 0x80)      = 0x000800B4
                [destino+0]         = 0x4EF90008
                [destino+4]         = 0x21AC4EF9
                [destino+8]         = 0x0000068E
                disparando trap #0 (getpid)
                voltou, d0            = 0x000003E8
                T3: fim 

## 2026/10/12 kernel e tstraw compilados com sucesso e rodando com sucesso
              tstraw provou que a aplicação anterior não estava rodando por
              que estava sendo compilada com newlib, por algum motivo isso faz
              travar. mas tstraw sem lib alguma funcionou e mostrou mensagem
              RAW OK
              main    = 0x000920B4
              global  = 0x0009215C
              sp      = 0x000D1FE8
 