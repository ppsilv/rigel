/*
 * ftest.c -- testa as syscalls de arquivo FAT (SYS_F*) do Rigel.
 * Cria /ftest.txt, le de volta, faz seek, stat, rename, findfirst e apaga.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>      /* unlink */
#include <rigel/rigel.h>

static void check(const char *nome, long r, int ok)
{
    printf("%-28s r=%ld  %s\n", nome, r, ok ? "OK" : "FALHOU");
}

int main(void)
{
    char buf[32];
    rigel_dirent_t e;
    long h, r;

    h = rigel_ff_open("/ftest.txt", RIGEL_FA_WRITE | RIGEL_FA_CREATE_ALWAYS);
    check("ff_open (criar)", h, h >= 0);
    if (h < 0) return 1;

    r = rigel_ff_write(h, "Ola Rigel 0123456789", 20);
    check("ff_write 20 bytes", r, r == 20);
    check("size", rigel_size(h), rigel_size(h) == 20);
    check("tell", rigel_tell(h), rigel_tell(h) == 20);
    check("eof (fim?)", rigel_eof(h), rigel_eof(h) == 1);
    check("ff_sync", rigel_ff_sync(h), rigel_ff_sync(h) == 0);
    check("ff_close", rigel_ff_close(h), 1);
    rigel_ff_close(h);

    h = rigel_ff_open("/ftest.txt", RIGEL_FA_READ);
    check("ff_open (ler)", h, h >= 0);
    r = rigel_ff_lseek(h, 5, RIGEL_SEEK_SET);
    check("lseek SET 5", r, r == 5);
    memset(buf, 0, sizeof buf);
    r = rigel_ff_read(h, buf, 5);
    check("ff_read 5", r, r == 5 && memcmp(buf, "Rigel", 5) == 0);
    printf("  leu: \"%s\"\n", buf);
    r = rigel_ff_lseek(h, -3, RIGEL_SEEK_END);
    check("lseek END -3", r, r == 17);
    rigel_ff_close(h);

    r = rigel_stat("/ftest.txt", &e);
    check("stat", r, r == 0 && e.size == 20);
    r = rename("/ftest.txt", "/ftest2.txt");
    check("rename", r, r == 0);

    h = rigel_findfirst("/", "ftest*", &e);
    check("findfirst ftest*", h, h >= 0);
    if (h >= 0) {
        printf("  achou: %s (%lu bytes)\n", e.name, (unsigned long)e.size);
        rigel_closedir(h);
    }

    r = rigel_ff_open("/ftest2.txt", RIGEL_FA_READ | RIGEL_FA_WRITE);
    if (r >= 0) {
        rigel_ff_lseek(r, 10, RIGEL_SEEK_SET);
        check("ioctl TRUNCATE em 10", rigel_ff_ioctl(r, RIGEL_FIOC_TRUNCATE, 0), 1);
        check("size apos truncate", rigel_size(r), rigel_size(r) == 10);
        rigel_ff_close(r);
    }

    r = unlink("/ftest2.txt");
    check("unlink", r, r == 0);
    check("stat (deve falhar)", rigel_stat("/ftest2.txt", &e), rigel_stat("/ftest2.txt", &e) == -1);
    printf("ftest terminou\n");
    return 0;
}