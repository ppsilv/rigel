/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_EEPROM_H
#define RIGEL_EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_eeprom_readbyte(long addr);
long rigel_eeprom_writebyte(long addr, long value);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_EEPROM_H */
