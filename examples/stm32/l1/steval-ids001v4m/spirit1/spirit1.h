#ifndef SPIRIT1_H
#define SPIRIT1_H

// Commands
#define SP1_CMD_TX 0x60
#define SP1_CMD_RX 0x61
#define SP1_CMD_READY 0x62
#define SP1_CMD_STANDBY 0x63
#define SP1_CMD_SLEEP 0x64
#define SP1_CMD_SRES 0x70
#define SP1_CMD_LOCKRX 0x65
#define SP1_CMD_LOCKTX 0x66
#define SP1_CMD_SABORT 0x67
#define SP1_CMD_FLUSHRXFIFO 0x71
#define SP1_CMD_FLUSHTXFIFO 0x72

//Registers
#define SP1_ANA_FUNC_CONF 0x00 //16bits
#define SP1_MC_STATE 0xC0 //16bits

#define SP1_FIFO 0xFF

//MC_STATE flags
#define MC_STATE_STATE_FLAG 0x00FE
#define SP1_STATE(x) (( MC_STATE_STATE_FLAG & (x) ) >> 1)

//STATES
#define SP1_ST_STANDBY 0x40
#define SP1_ST_SLEEP 0x36
#define SP1_ST_READY 0x03
#define SP1_ST_LOCK 0x0F
#define SP1_ST_RX 0x33
#define SP1_ST_TX 0x5f

//TODO make struct to add gpioport, gpios and spiport for all functions

char* get_state_str(uint8_t state);
void print_sp1_status(uint16_t status);
uint16_t my_spi_xfer(uint32_t spi, uint16_t data);
uint16_t sp1_write(uint8_t reg_addr, uint8_t *wr_data, uint8_t count,
                   uint32_t spiport, uint32_t gpioport, uint16_t gpios);
uint16_t sp1_cmd(uint8_t cmd, uint32_t spiport,
                 uint32_t gpioport, uint16_t gpios);
uint16_t sp1_read(uint8_t reg_addr, uint8_t *rd_data, uint8_t count,
                  uint32_t spiport, uint32_t gpioport, uint16_t gpios);
void sp1_spi_setup(uint32_t spiport);

#endif //SPIRIT1_H
