#ifndef STEVAL_IDS001V4M_H
#define STEVAL_IDS001V4M_H

/* uint16_t spsgrf_write(uint8_t reg_addr, uint8_t *wr_data, uint8_t count); */
/* uint16_t spsgrf_cmd(uint8_t cmd); */
/* uint16_t spsgrf_read(uint8_t reg_addr, uint8_t *rd_data, uint8_t count, */
/*                      bool inv_dir); */

void spi_setup(void);
void spsgrf868_setup(void);
void eeprom_init(void);

#endif // STEVAL_IDS001V4M_H
