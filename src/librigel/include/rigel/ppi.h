/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_PPI_H
#define RIGEL_PPI_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_ppia_cfg(long mode);
long rigel_ppi_porta_read(void);
long rigel_ppi_porta_write(long value);
long rigel_ppib_cfg(long mode);
long rigel_ppi_portb_read(void);
long rigel_ppi_portb_write(long value);
long rigel_ppic_cfg(long mode);
long rigel_ppi_portc_read(void);
long rigel_ppi_portc_write(long value);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_PPI_H */
