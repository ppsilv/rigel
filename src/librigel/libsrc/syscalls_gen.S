/* GERADO por tools/gen_librigel.py a partir do syscall.h -- NAO EDITE. */
        .macro  SYSCALL name, num
        .global \name
\name:
        move.l  #\num,%d0
        trap    #0
        rts
        .endm

        .text
        SYSCALL _exit                        0x0000   /* SYS_EXIT */
        SYSCALL open                         0x0048   /* SYS_OPEN */
        SYSCALL close                        0x0049   /* SYS_CLOSE */
        SYSCALL read                         0x004A   /* SYS_READ */
        SYSCALL write                        0x004B   /* SYS_WRITE */
        SYSCALL lseek                        0x004D   /* SYS_LSEEK */
        SYSCALL getpid                       0x0044   /* SYS_GETPID */
        SYSCALL unlink                       0x000C   /* SYS_FUNLINK */
        SYSCALL rename                       0x0011   /* SYS_FRENAME */
        SYSCALL mkdir                        0x0017   /* SYS_FMKDIR */
        SYSCALL rigel_spawn                  0x0001   /* SYS_SPAWN */
        SYSCALL rigel_waitpid                0x004F   /* SYS_WAITPID */
        SYSCALL rigel_sleep                  0x0002   /* SYS_SLEEP */
        SYSCALL rigel_yield                  0x0003   /* SYS_YIELD */
        SYSCALL rigel_setramvector           0x0045   /* SYS_SETRAMVECTOR */
        SYSCALL rigel_fsync                  0x004C   /* SYS_SINC */
        SYSCALL rigel_ioctl                  0x004E   /* SYS_IOCTL */
        SYSCALL rigel_dup                    0x000B   /* SYS_DUP */
        SYSCALL rigel_chmod                  0x000D   /* SYS_CHMOD */
        SYSCALL rigel_tell                   0x000E   /* SYS_FTELL */
        SYSCALL rigel_size                   0x000F   /* SYS_FSIZE */
        SYSCALL rigel_eof                    0x0010   /* SYS_FEOF */
        SYSCALL rigel_stat                   0x0012   /* SYS_FSTAT */
        SYSCALL rigel_opendir                0x0013   /* SYS_FOPENDIR */
        SYSCALL rigel_readdir                0x0014   /* SYS_FREADDIR */
        SYSCALL rigel_mount                  0x0015   /* SYS_FMOUNT */
        SYSCALL rigel_closedir               0x0016   /* SYS_FCLOSEDIR */
        SYSCALL rigel_unmount                0x0018   /* SYS_FUNMOUNT */
        SYSCALL rigel_findfirst              0x0019   /* SYS_FFINDFIRST */
        SYSCALL rigel_findnext               0x001A   /* SYS_FFINDNEXT */
        SYSCALL rigel_ff_open                0x0004   /* SYS_FOPEN */
        SYSCALL rigel_ff_close               0x0005   /* SYS_FCLOSE */
        SYSCALL rigel_ff_read                0x0006   /* SYS_FREAD */
        SYSCALL rigel_ff_write               0x0007   /* SYS_FWRITE */
        SYSCALL rigel_ff_sync                0x0008   /* SYS_FSYNC */
        SYSCALL rigel_ff_lseek               0x0009   /* SYS_FLSEEK */
        SYSCALL rigel_ff_ioctl               0x000A   /* SYS_FIOCTL */
        SYSCALL rigel_vga_putchar            0x001B   /* SYS_VGAPUTCHAR */
        SYSCALL rigel_gotoxy                 0x001C   /* SYS_GOTOXY */
        SYSCALL rigel_gohome                 0x001D   /* SYS_GOHOME */
        SYSCALL rigel_settextcolor           0x001E   /* SYS_SETTEXTCOLOR */
        SYSCALL rigel_clrscr                 0x001F   /* SYS_CLRSCR */
        SYSCALL rigel_drawvline              0x0020   /* SYS_DRAWVLINE */
        SYSCALL rigel_drawhline              0x0021   /* SYS_DRAWHLINE */
        SYSCALL rigel_drawline               0x0022   /* SYS_DRAWLINE */
        SYSCALL rigel_drawcircle             0x0023   /* SYS_DRAWCIRCLE */
        SYSCALL rigel_drawcirclehelper       0x0024   /* SYS_DRAWCIRCLEHELPER */
        SYSCALL rigel_fillcircle             0x0025   /* SYS_FILLCIRCLE */
        SYSCALL rigel_fillcirclehelper       0x0026   /* SYS_FILLCIRCLEHELPER */
        SYSCALL rigel_drawroundrect          0x0027   /* SYS_DRAWROUNDRECT */
        SYSCALL rigel_fillroundrect          0x0028   /* SYS_FILLROUNDRECT */
        SYSCALL rigel_fillrect               0x0029   /* SYS_FILLRECT */
        SYSCALL rigel_readtime               0x002A   /* SYS_READTIME */
        SYSCALL rigel_writetime              0x002B   /* SYS_WRITETIME */
        SYSCALL rigel_readrtc                0x0046   /* SYS_READRTC */
        SYSCALL rigel_writertc               0x0047   /* SYS_WRITERTC */
        SYSCALL rigel_eeprom_readbyte        0x002C   /* SYS_EEPROMREADBYTE */
        SYSCALL rigel_eeprom_writebyte       0x002D   /* SYS_EEPROMWRITEBYTE */
        SYSCALL rigel_ringbuffer_read        0x002E   /* SYS_RINGBUFFERREAD */
        SYSCALL rigel_ringbuffer_write       0x002F   /* SYS_RINGBUFFERWRITE */
        SYSCALL rigel_duarta_cfg             0x0030   /* SYS_DUARTACFG */
        SYSCALL rigel_duarta_read            0x0031   /* SYS_DUARTAREAD */
        SYSCALL rigel_duarta_write           0x0032   /* SYS_DUARTAWRITE */
        SYSCALL rigel_duartb_cfg             0x0033   /* SYS_DUARTBCFG */
        SYSCALL rigel_duartb_read            0x0034   /* SYS_DUARTBREAD */
        SYSCALL rigel_duartb_write           0x0035   /* SYS_DUARTBWRITE */
        SYSCALL rigel_ppia_cfg               0x0036   /* SYS_PPIACFG */
        SYSCALL rigel_ppi_porta_read         0x0037   /* SYS_PPIPORTAREAD */
        SYSCALL rigel_ppi_porta_write        0x0038   /* SYS_PPIPORTAWRITE */
        SYSCALL rigel_ppib_cfg               0x0039   /* SYS_PPIBCFG */
        SYSCALL rigel_ppi_portb_read         0x003A   /* SYS_PPIPORTBREAD */
        SYSCALL rigel_ppi_portb_write        0x003B   /* SYS_PPIPORTBWRITE */
        SYSCALL rigel_ppic_cfg               0x003C   /* SYS_PPICCFG */
        SYSCALL rigel_ppi_portc_read         0x003D   /* SYS_PPIPORTCREAD */
        SYSCALL rigel_ppi_portc_write        0x003E   /* SYS_PPIPORTCWRITE */
        SYSCALL rigel_connect                0x003F   /* SYS_CONNECT */
        SYSCALL rigel_bind                   0x0040   /* SYS_BIND */
        SYSCALL rigel_listen                 0x0041   /* SYS_LISTEN */
        SYSCALL rigel_accept                 0x0042   /* SYS_ACCEPT */
        SYSCALL rigel_setsockopt             0x0043   /* SYS_SETSOCKOPT */
        SYSCALL rigel_getkbdkey              0x0050   /* SYS_GETKBDKEY */
        SYSCALL rigel_getuptime              0x1050   /* SYS_GETUPTIME */
