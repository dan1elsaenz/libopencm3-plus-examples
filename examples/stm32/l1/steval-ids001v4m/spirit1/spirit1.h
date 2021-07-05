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
#define SP1_IF_OFFSET_ANA 0x07
#define SP1_IF_OFFSET_DIG 0x0D
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
#define SP1_FDEV0 0x1C
#define SP1_AFC2 0x1E
#define SP1_AFC1 0x1F
#define SP1_AFC0 0x20
#define SP1_AGCCTRL0 0x26
#define SP1_AGCCTRL1 0x25
#define SP1_AGCCTRL2 0x24
#define SP1_ANT_SELECT_CONF 0x27
#define SP1_PCKTCTRL1 0x33
#define SP1_PCKTCTRL2 0x32
#define SP1_PCKTCTRL3 0x31
#define SP1_PCKTCTRL4 0x30
#define SP1_PCKTLEN0 0x35
#define SP1_PCKTLEN1 0x34
#define SP1_PCKT_FLT_OPTIONS 0x4F
#define SP1_PROTOCOL0 0x52
#define SP1_PROTOCOL1 0x51
#define SP1_PROTOCOL2 0x50
#define SP1_TX_PCKT_INFO 0xC2
#define SP1_RX_PCKT_INFO 0xC3
#define SP1_AFC_CORR 0xC4
#define SP1_LINK_QUALIF2 0xC5
#define SP1_LINK_QUALIF1 0xC6
#define SP1_LINK_QUALIF0 0xC7
#define SP1_RSSI_LEVEL 0xC8
#define SP1_RX_PCKT_LEN1 0xC9
#define SP1_RX_PCKT_LEN0 0xCA
#define SP1_LINEAR_FIFO_STATUS1 0xE6
#define SP1_LINEAR_FIFO_STATUS0 0xE7
#define SP1_DEVICE_INFO1 0xF0
#define SP1_DEVICE_INFO0 0xF1
#define SP1_FIFO 0xFF

#define SP1_FIFO_SIZE 96

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

// Frequency deviation flags
#define SP1_FDEV0_E (0xF << 4)
#define SP1_FDEV0_CLOCK_REC_ALGO_SEL (1 << 3)
#define SP1_FDEV0_M (0x7 << 0)

// AFC2 flags
#define SP1_AFC2_FREEZE_ON_SYNC (1 << 7)
#define SP1_AFC2_ENABLE (1 << 6)
#define SP1_AFC2_MODE (1 << 5)
#define SP1_AFC2_PD_LEAKAGE (0x1F << 0)

// AGC flags
#define SP1_AGCCTRL0_ENABLE (1 << 7)
#define SP1_AGCCTRL1_TH_HIGH (0xF << 4)
#define SP1_AGCCTRL1_TH_LOW (0xF << 0)
#define SP1_AGCCTRL2_MEAS_TIME (0xF << 0)

// Ant Sel Conf flags
#define SP1_ANT_SELECT_CONF_CS_BLANKING (1 << 4)
#define SP1_ANT_SELECT_CONF_ENABLE (3 << 4)
#define SP1_ANT_SELECT_CONF_MEAS_TIME (0x7 << 0)

// PCKTCTRL flags
#define SP1_PCKTCTRL1_CRC_MODE (0x7 << 5)
#define SP1_PCKTCTRL1_WHIT_EN (1 << 4)
#define SP1_PCKTCTRL1_TXSOURCE (0x3 << 2)
#define SP1_PCKTCTRL1_FEC_EN (1 << 0)
#define SP1_PCKTCTRL2_PREAMBLE_LENGTH (0x1F << 3)
#define SP1_PCKTCTRL2_SYNC_LENGTH (0x3 << 1)
#define SP1_PCKTCTRL2_FIX_VAR_LEN (1 << 0)
#define SP1_PCKTCTRL3_PCKT_FRMT (0x3 << 6)
#define SP1_PCKTCTRL3_RX_MODE (0x3 << 4)
#define SP1_PCKTCTRL3_LEN_WID (0xF << 0)
#define SP1_PCKTCTRL4_ADDRESS_LEN (0x3 << 3)
#define SP1_PCKTCTRL4_CONTROL_LEN (0x7 << 0)

// PROTOCOL flags
#define SP1_PROTOCOL0_NMAX_RETX (0xF << 4)
#define SP1_PROTOCOL0_NACK_TX (1 << 3)
#define SP1_PROTOCOL0_AUTO_ACK (1 << 2)
#define SP1_PROTOCOL0_PERS_RX (1 << 1)
#define SP1_PROTOCOL0_PERS_TX (1 << 0)
#define SP1_PROTOCOL1_LDC_RELOAD_ON_SYNC (1 << 7)
#define SP1_PROTOCOL1_PIGGYBACKING (1 << 6)
#define SP1_PROTOCOL1_SEED_RELOAD (1 << 3)
#define SP1_PROTOCOL1_CSMA_ON (1 << 2)
#define SP1_PROTOCOL1_CSMA_PERS_ON (1 << 1)
#define SP1_PROTOCOL1_AUTO_PCKT_FLT (1 << 0)
#define SP1_PROTOCOL2_CS_TIMEOUT_MASK (1 << 7)
#define SP1_PROTOCOL2_SQI_TIMEOUT_MASK (1 << 6)
#define SP1_PROTOCOL2_PQI_TIMEOUT_MASK (1 << 5)
#define SP1_PROTOCOL2_TX_SEQ_NUM_RELOAD (0x3 << 3)
#define SP1_PROTOCOL2_RCO_CALIBRATION (1 << 2)
#define SP1_PROTOCOL2_VCO_CALIBRATION (1 << 1)
#define SP1_PROTOCOL2_LDC_MODE (1 << 0)

// TX pckt info flags
#define SP1_TX_PCKT_INFO_TX_SEQ_NUM (0x3 << 4)
#define SP1_TX_PCKT_INFO_N_RETX (0xF << 0)

// TX pckt info flags
#define SP1_RX_PCKT_INFO_NACK_RX (1 << 2)
#define SP1_RX_PCKT_INFO_RX_SEQ_NUM (0x3 << 0)

// Link qualif flags
#define SP1_LINK_QUALIF1_CS (1 << 7)
#define SP1_LINK_QUALIF1_SQI (0x7F << 0)
#define SP1_LINK_QUALIF0_AGC_WORD (0xF << 0)

// Linear fifo status flags
#define SP1_LINEAR_FIFO_STATUS1_ELEM_TXFIFO (0x7F << 0)
#define SP1_LINEAR_FIFO_STATUS0_ELEM_RXFIFO (0x7F << 0)

// STATES
#define SP1_ST_STANDBY 0x40
#define SP1_ST_SLEEP 0x36
#define SP1_ST_READY 0x03
#define SP1_ST_LOCK 0x0F
#define SP1_ST_RX 0x33
#define SP1_ST_TX 0x5f

typedef enum {
  RX_TIMEOUT_AND_OR_SELECT = (1 << 6),
  CONTROL_FILTERING = (1 << 5),
  SOURCE_FILTERING = (1 << 4),
  DEST_VS_SOURCE_ADDR = (1 << 3),
  DEST_VS_MULTICAST_ADDR = (1 << 2),
  DEST_VS_BROADCAST_ADDR = (1 << 1),
  CRC_CHECK = (1 << 0)
} Pckt_flt_options;

typedef enum {
  AFC_SLICER, //
  AFC_2ND_CONV_STAGE
} Afc_mode;

typedef enum {
  PCKT_CRC_NONE,
  PCKT_CRC_0x07,
  PCKT_CRC_0x8005,
  PCKT_CRC_0x1021,
  PCKT_CRC_0x864CBF
} Pckt_crc_mode;

typedef enum {
  PCKT_FRMT_Basic = 0x0,
  PCKT_FRMT_WM_bus = 0x2,
  PCKT_FRMT_STack = 0x3,
} Pckt_frmt;

typedef enum {
  PCKT_RX_MODE_Normal = 0x0,
  PCKT_RX_MODE_DIRECT_FIFO = 0x1,
  PCKT_RX_MODE_DIRECT_GPIO = 0x2,
} Pckt_rx_mode;

typedef enum {
  PCKT_FIX_LEN = 0x0,
  PCKT_VAR_LEN = 0x1,
} Pckt_fix_var;

typedef struct {
  uint32_t spiport;
  uint32_t gpioport;
  uint16_t spi_cs;
  uint32_t sdnport;
  uint16_t sdnpin;
  double fxo;
} SpiritSPI;

typedef struct {
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
  double h_index;
  uint8_t chflt_m;
  uint8_t chflt_e;
  bool afc;
  bool afc_freeze_on_sync;
  Afc_mode afc_mode;
  bool agc;
  uint8_t agc_th_high;
  uint8_t agc_th_low;
  bool ant_sel_cs_blanking;
  bool pckt_whitening;
  Pckt_crc_mode pckt_crc_mode;
  uint8_t pckt_preamble_len;
  Pckt_fix_var pckt_fix_var;
  uint16_t pckt_len;
  Pckt_flt_options pckt_flt_options;
  uint8_t protocol_nmax_retx;
  bool protocol_nack_tx;
  bool protocol_auto_ack;
  bool protocol_pers_rx;
  bool protocol_pers_tx;
  bool protocol_ldc_reload_on_sync;
  bool protocol_piggybacking;
  bool protocol_seed_reload;
  bool protocol_csma_on;
  bool protocol_csma_pers_on;
  bool protocol_auto_pckt_flt;
  bool protocol_cs_timeout_mask;
  bool protocol_sqi_timeout_mask;
  bool protocol_pqi_timeout_mask;
  uint8_t protocol_tx_seq_num_reload;
  bool protocol_rco_calib;
  bool protocol_vco_calib;
  bool protocol_ldc_mode;
  Pckt_frmt pckt_frmt;
  uint8_t pckt_addr_len;
  Pckt_rx_mode pckt_rx_mode;
  uint8_t partnum;
  uint8_t version;
} SpiritConf;

typedef struct dwrite {
  uint8_t reg;
  uint8_t data;
} Data_write;

// General setting up functions
void min_init(SpiritSPI dev);
void sp1_spi_setup(SpiritSPI spi_conf);
void init_spirit_spi(SpiritSPI dev);
void init_spirit(SpiritSPI dev, SpiritConf conf);

// Digital clock and Intermediate freq. functions
double get_fclk(SpiritSPI dev);
void set_clkdiv(SpiritSPI dev);
double calc_if_ana(SpiritSPI dev);
double calc_if_dig(SpiritSPI dev);
void set_if(SpiritSPI dev);

// Base frequency, channel, channel spacing functions
double set_fbase(SpiritSPI dev, SpiritConf *conf);
void _get_fbase(SpiritSPI dev);
double get_fbase(SpiritSPI dev);
void set_channel(SpiritSPI dev, SpiritConf conf);
void set_ch_space_steps(SpiritSPI dev, SpiritConf conf);
double get_fchannel(SpiritSPI dev);
double _get_channel_spacing(SpiritSPI dev);
uint8_t _get_channel(SpiritSPI dev);
uint32_t _get_synt_from_reg(SpiritSPI dev);
void set_synth_refdiv(SpiritSPI dev, int D);
void set_synt_reg(SpiritSPI dev, uint32_t synt);
void set_tsplit(SpiritSPI dev, SpiritConf conf);
uint8_t _get_B(SpiritSPI dev);
uint8_t _get_D(SpiritSPI dev);
int16_t _get_foffset(SpiritSPI dev);

// Output power
void tx_ramp(SpiritSPI dev, SpiritConf conf);
void set_tx_ramp_max_index(SpiritSPI dev, SpiritConf conf);
void set_tx_ramp_step_width(SpiritSPI dev, SpiritConf conf);
float get_tx_power(SpiritSPI dev, uint8_t slot);
void set_tx_power(SpiritSPI dev, SpiritConf conf);
void set_tx_out_capis(SpiritSPI dev, SpiritConf conf);

// Reception functions
void set_chflt(SpiritSPI dev, SpiritConf conf);
void print_chflt(SpiritSPI dev);
uint8_t get_rx_afc_corr(SpiritSPI dev);
uint8_t get_rx_PQI(SpiritSPI dev);
uint8_t get_rx_cs_indication(SpiritSPI dev);
uint8_t get_rx_SQI(SpiritSPI dev);
uint8_t get_rx_agc_word(SpiritSPI dev);
uint8_t get_rx_RSSI_level(SpiritSPI dev);
uint16_t get_rx_pckt_len(SpiritSPI dev);
void set_cs_blanking(SpiritSPI dev, SpiritConf conf);

// Protocol and packet functions
void set_protocol_flags(SpiritSPI dev, SpiritConf conf);
void set_pckt_flt_options(SpiritSPI dev, SpiritConf conf);
void set_pckt_len(SpiritSPI dev, SpiritConf conf);
void set_pckt_fix_var_len(SpiritSPI dev, SpiritConf conf);
void set_pckt_preamble_len(SpiritSPI dev, SpiritConf conf);
void set_pckt_crc_mode(SpiritSPI dev, SpiritConf conf);
void set_pckt_whitening(SpiritSPI dev, SpiritConf conf);
void set_pckt_format(SpiritSPI dev, SpiritConf conf);
void set_pckt_rx_mode(SpiritSPI dev, SpiritConf conf);
void set_pckt_addr_len(SpiritSPI dev, SpiritConf conf);
uint8_t get_tx_seq_num(SpiritSPI dev);
uint8_t get_n_retx(SpiritSPI dev);
uint8_t get_rx_seq_num(SpiritSPI dev);
uint8_t get_nack_rx(SpiritSPI dev);

// AGC RX Auto gain control functions
void set_agc_th_high(SpiritSPI dev, SpiritConf conf);
void set_agc_th_low(SpiritSPI dev, SpiritConf conf);
void agc_enable(SpiritSPI dev, SpiritConf conf);

// AFC RX Auto Frequency Compensation
void afc_freeze_on_sync(SpiritSPI dev, SpiritConf conf);
void afc_enable(SpiritSPI dev, SpiritConf conf);
void afc_mode(SpiritSPI dev, SpiritConf conf);

// Modulation and Datarate functions
void set_mod_type(SpiritSPI dev, SpiritConf conf);
uint8_t get_mod_type(SpiritSPI dev);
void set_datarate(SpiritSPI dev, SpiritConf *conf);
double get_datarate(SpiritSPI dev);
double _calc_frequency_deviation(float datarate, float H);
double get_frequency_deviation(SpiritSPI dev);
void set_frequency_deviation(SpiritSPI dev, SpiritConf conf);

// Calibraion functions
void rco_calib(SpiritSPI dev, bool enable);
void vco_calib(SpiritSPI dev, bool enable);

// Utils functions
void _update_bitfield(SpiritSPI dev, uint8_t reg, uint8_t bitfield,
                      uint8_t value);
uint8_t _get_bitfield(SpiritSPI dev, uint8_t reg, uint8_t bitfield);
void write_many(SpiritSPI dev, Data_write *list, int n);
uint16_t my_spi_xfer(uint32_t spi, uint16_t data);
uint16_t sp1_write(SpiritSPI spi_conf, uint8_t reg_addr,
                   uint8_t *wr_data, uint8_t count);
uint16_t sp1_read(SpiritSPI spi_conf, uint8_t reg_addr,
                  uint8_t *rd_data, uint8_t count, bool inv_dir);

// Spirit util functions
void get_device_info(SpiritSPI dev, SpiritConf *conf);
void print_device_info(SpiritConf conf);
uint16_t get_mc_state(SpiritSPI dev);
char *get_state_str(uint8_t state);
void print_sp1_status(uint16_t status);
uint16_t sp1_cmd(SpiritSPI spi_conf, uint8_t cmd);
void change_to_state(SpiritSPI dev, int state_cmd, int state_result);
void wait_state(SpiritSPI dev, uint8_t state);

// Buffer functions
uint8_t get_elem_txfifo(SpiritSPI dev);
uint8_t get_elem_rxfifo(SpiritSPI dev);
void write_buffer(SpiritSPI dev, SpiritConf *conf, unsigned char *buf,
                  uint8_t max_count);
void read_buffer(SpiritSPI dev, unsigned char *buf, int count);

#endif // SPIRIT1_H
