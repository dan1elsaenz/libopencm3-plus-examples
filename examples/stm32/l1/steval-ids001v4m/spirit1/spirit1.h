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

// Registers
#define SP1_ANA_FUNC_CONF 0x00      // 16bits
#define SP1_MC_STATE 0xC0           // 16bits
#define SP1_LINEAR_FIFO_STATUS 0xE6 // 16bits
#define SP1_DEM_CONFIG 0xA3
#define SP1_XO_RCO_TEST 0xB4
#define SP1_SYNT0 0x0B
#define SP1_SYNT1 0x0A
#define SP1_SYNT2 0x09
#define SP1_SYNT3 0x08
#define SP1_SYNTH_CONFIG1 0x9E
#define SP1_SYNTH_CONFIG0 0x9F
#define SP1_CHSPACE 0x0C
#define SP1_FIFO 0xFF

// Flags

// MC_STATE flags
#define SP1_MC_STATE_STATE_FLAG 0x00FE
#define SP1_STATE(x) ((SP1_MC_STATE_STATE_FLAG & (x)) >> 1)

// LINEAR_FIFO_STATUS flags
#define SP1_LINEAR_FIFO_STATUS_ELEM_TXFIFO 0x7F00
#define SP1_LINEAR_FIFO_STATUS_ELEM_RXFIFO 0x007F
#define SP1_LINEAR_FIFO_STATUS_RXCOUNT(x)                            \
  ((SP1_LINEAR_FIFO_STATUS_ELEM_RXFIFO & (x)) >> 0)

// DEM_CONFIG flags
#define SP1_DEM_CONFIG_DEM_ORDER (1 << 1)

// XO_RCO_TEST flags
#define SP1_XO_RCO_TEST_PD_CLKDIV (1 << 3)

// SYNT0 flags
#define SP1_SYNT0_SYNT4_0 3 // bit pos
#define SP1_SYNT0_BS (0x7 << 0)

// SYNT3 flags
#define SP1_SYNT3_WCP (0x7 << 5)

// SYNTH_CONFIG1 flags
#define SP1_SYNTH_CONFIG1_REFDIV (1 << 7)

// STATES
#define SP1_ST_STANDBY 0x40
#define SP1_ST_SLEEP 0x36
#define SP1_ST_READY 0x03
#define SP1_ST_LOCK 0x0F
#define SP1_ST_RX 0x33
#define SP1_ST_TX 0x5f

typedef struct {
  uint32_t spiport;
  uint32_t gpioport;
  uint16_t spi_cs;
  uint32_t sdnport;
  uint16_t sdnpin;
  float fxo;
  float fbase;
  uint8_t ch_space_steps;
} SpiritSPI;

typedef struct dwrite {
  uint8_t reg;
  uint8_t data;
} Data_write;

void min_init(SpiritSPI dev);
void change_to_state(SpiritSPI dev, int state_cmd, int state_result);
float get_fclk(SpiritSPI dev);
void set_clkdiv(SpiritSPI dev);
void set_ch_space_steps(SpiritSPI dev, uint8_t steps);
void set_synth_refdiv(SpiritSPI dev, int D);
void set_synt_reg(SpiritSPI dev, uint32_t synt);
float set_synt(SpiritSPI dev);
float calc_if_ana(SpiritSPI dev);
float calc_if_dig(SpiritSPI dev);
uint16_t get_mc_state(SpiritSPI dev);
void write_many(SpiritSPI dev, Data_write *list, int n);
void read_buffer(SpiritSPI dev, unsigned char *buf, int count);
char *get_state_str(uint8_t state);
void print_sp1_status(uint16_t status);
uint16_t my_spi_xfer(uint32_t spi, uint16_t data);
uint16_t sp1_write(SpiritSPI spi_conf, uint8_t reg_addr,
                   uint8_t *wr_data, uint8_t count);
uint16_t sp1_cmd(SpiritSPI spi_conf, uint8_t cmd);
uint16_t sp1_read(SpiritSPI spi_conf, uint8_t reg_addr,
                  uint8_t *rd_data, uint8_t count, bool inv_dir);
void sp1_spi_setup(SpiritSPI spi_conf);

#endif // SPIRIT1_H
