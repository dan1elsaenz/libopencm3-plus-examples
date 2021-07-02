#ifndef SPIRIT1_H
#define SPIRIT1_H

// Formulas
#define FREQ_CH(fbase, foffset, fxo, ch_space_step, channel)

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
#define SP1_CHNUM 0x6C
#define SP1_FC_OFFSET0 0x0F
#define SP1_FC_OFFSET1 0x0E
#define SP1_PA_POWER0 0x18
#define SP1_PA_POWER1 0x17
#define SP1_PA_POWER2 0x16
#define SP1_PA_POWER3 0x15
#define SP1_PA_POWER4 0x14
#define SP1_PA_POWER5 0x13
#define SP1_PA_POWER6 0x12
#define SP1_PA_POWER7 0x11
#define SP1_PA_POWER8 0x10
#define SP1_MOD1 0x1A
#define SP1_MOD0 0x1B
#define SP1_CHFLT 0x1D
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

// SYNTH_CONFIG0 flags
// 0: 1.73ns, 1: 3.47ns
#define SP1_SYNTH_CONFIG0_SEL_TSPLIT (1 << 7)

// PA_POWER0 flags
#define SP1_PA_POWER0_CWC (0x3 << 6)
#define SP1_PA_POWER0_CWC_0pF 0x00
#define SP1_PA_POWER0_CWC_1pF2 0x01
#define SP1_PA_POWER0_CWC_2pF4 0x02
#define SP1_PA_POWER0_CWC_3pF6 0x03
#define SP1_PA_POWER0_RAMP_ENABLE (0x1 << 5)
#define SP1_PA_POWER0_RAMP_STEP_WIDTH (0x3 << 3)
#define SP1_PA_POWER0_LEVEL_MAX_INDEX (0x7 << 0)

// MOD0 flags
#define SP1_MOD0_CW (1 << 7)
#define SP1_MOD0_BT_SEL (1 << 6)
#define SP1_MOD0_BT1 0x0
#define SP1_MOD0_BT2 0x1
#define SP1_MOD0_MOD_TYPE (0x3 << 4)
#define SP1_MOD0_MOD_TYPE_2_FSK 0x0
#define SP1_MOD0_MOD_TYPE_GFSK 0x1
#define SP1_MOD0_MOD_TYPE_ASK_OOK 0x2
#define SP1_MOD0_MOD_TYPE_MSK 0x3
#define SP1_MOD0_DATARATE_E (0xF << 0)

// CHFLT flags
#define SP1_CHFLT_M (0xF << 4)
#define SP1_CHFLT_E (0xF << 0)

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
  double fxo;
  double fbase_cmd;
  double fbase_rd;
  uint8_t ch_space_steps;
  uint8_t channel;
  uint8_t tsplit;
  float tx_power[8];
  uint8_t tx_out_capis;
  bool tx_ramp;
  uint8_t tx_ramp_max_index;
  uint8_t tx_ramp_step;
  double datarate_cmd;
  double datarate_rd;
  uint8_t mod_type;
  uint8_t chflt_m;
  uint8_t chflt_e;

} SpiritSPI;

typedef struct dwrite {
  uint8_t reg;
  uint8_t data;
} Data_write;

void min_init(SpiritSPI dev);
void change_to_state(SpiritSPI dev, int state_cmd, int state_result);
void set_mod_type(SpiritSPI dev);
uint8_t get_mod_type(SpiritSPI dev);
void set_datarate(SpiritSPI *dev);
double get_datarate(SpiritSPI dev);
double get_fclk(SpiritSPI dev);
void set_clkdiv(SpiritSPI dev);
double get_fchannel(SpiritSPI dev);
double _get_channel_spacing(SpiritSPI dev);
int16_t _get_foffset(SpiritSPI dev);
uint8_t _get_channel(SpiritSPI dev);
uint32_t _get_synt_from_reg(SpiritSPI dev);
void _update_bitfield(SpiritSPI dev, uint8_t reg, uint8_t bitfield,
                      uint8_t value);
uint8_t _get_bitfield(SpiritSPI dev, uint8_t reg, uint8_t bitfield);
uint8_t _get_B(SpiritSPI dev);
uint8_t _get_D(SpiritSPI dev);
void _get_fbase(SpiritSPI dev);
double get_fbase(SpiritSPI dev);
void set_channel(SpiritSPI dev);
void set_ch_space_steps(SpiritSPI dev, uint8_t steps);
void set_synth_refdiv(SpiritSPI dev, int D);
void set_synt_reg(SpiritSPI dev, uint32_t synt);
double set_fbase(SpiritSPI *dev);
double calc_if_ana(SpiritSPI dev);
double calc_if_dig(SpiritSPI dev);
void set_tsplit(SpiritSPI dev);
void tx_ramp(SpiritSPI dev, bool enable);
void set_tx_ramp_max_index(SpiritSPI dev);
void set_tx_ramp_step_width(SpiritSPI dev);
float get_tx_power(SpiritSPI dev, uint8_t slot);
void set_tx_power(SpiritSPI dev, uint8_t slot);
void set_tx_out_capis(SpiritSPI dev);
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
