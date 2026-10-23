/* kill.c - uso: kill [-SINAL] pid [pid ...]   (SINAL: 0, 9 ou 15) */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/sys_ctl.h>

int main(int argc, char **argv)
{
    unsigned sig = SIGKILL;
    int i = 1, rc = 0;

    if (argc > 1 && argv[1][0] == '-') {
        char *end;
        long s = strtol(argv[1] + 1, &end, 10);
        if (end == argv[1] + 1 || *end != '\0' || s < 0 || s > 255) {
            printf("kill: sinal invalido: %s\n", argv[1]);
            return 1;
        }
        sig = (unsigned)s;
        i = 2;
    }

    if (i >= argc) {
        printf("uso: kill [-SINAL] pid [pid ...]\n");
        return 1;
    }

    for (; i < argc; i++) {
        char *end;
        long pid = strtol(argv[i], &end, 10);
        int r;

        if (end == argv[i] || *end != '\0' || pid <= 0 || pid > 0x7FFFFFFFL) {
            printf("kill: pid invalido: %s\n", argv[i]);
            rc = 1;
            continue;
        }

        r = rigel_kill((unsigned)pid, sig);
        if (r == K_ESRCH) {
            printf("kill: (%ld) processo nao existe\n", pid);
            rc = 1;
        } else if (r == K_EPERM) {
            printf("kill: (%ld) task do kernel, nao pode ser morta\n", pid);
            rc = 1;
        } else if (r < 0) {
            printf("kill: (%ld) erro %d\n", pid, r);
            rc = 1;
        }
    }
    return rc;
}
