#ifndef SPIRIT1_H
#define SPIRIT1_H

//TODO make struct to add gpioport, gpios and spiport for all functions

uint16_t my_spi_xfer(uint32_t spi, uint16_t data);
uint16_t sp1_write(uint8_t reg_addr, uint8_t *wr_data, uint8_t count,
                   uint32_t spiport, uint32_t gpioport, uint16_t gpios);
uint16_t sp1_cmd(uint8_t cmd, uint32_t spiport,
                 uint32_t gpioport, uint16_t gpios);
uint16_t sp1_read(uint8_t reg_addr, uint8_t *rd_data, uint8_t count,
                  uint32_t spiport, uint32_t gpioport, uint16_t gpios);
void sp1_spi_setup(uint32_t spiport);

#endif //SPIRIT1_H
