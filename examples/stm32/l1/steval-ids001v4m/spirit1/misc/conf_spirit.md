# SPIRIT

DEM_CONFIG
0xA3: 0x35 -> DEM_ORDER to 0 during radio init


(Table 31): f_if must be 480kHz, for f_xo=50MHz, these are the recommended values:
0x07 IF_OFFSET_ANA: 0x36
0x0D IF_OFFSET_DIG: 0xAC

## Osc and RF synth:

- Charge pump

(table 26)
```
WCP[2:0] 000 (860.2-8720)
```

With WCP you select the channel ! 000 is the center channel. It is a signed 
number!

- Equation 4:

For f_base = 868000475.5655924
```
SYNT: 0x6828F*2**5+0b10011
```

Synt = (fbase/fxo) * (2^18)* (B*D/2)

SYNT:
0x08: 06 (WCP = 000: CP current depends on VCO) Table 26
0x09: 82
0x0A: 8F
0x0B: A9 -> 99  (BS = 001 == high band) (synt band select)

FC_OFFSET = 0

SYNT is the PLL for the middle frequency. It selects the frequency right at
the center of the middle channel (WCP==000)

(CHSPACE)
0x0C: 01  (One step == 1526Hz (@50MHz/fxo))

(CHNUM)
0x6C: 0x00 Channel0

VCO_H_SEL=>1 (default)

For fxo == 50MHz ==> f_dig == fxo/2

SYNTH_CONFIG.SEL_TSPLIT == 1  (Recommended to facilitate the VCO calibrator operation)
0x9F: A0

SYNTH_CONFIG.REFDIV not divided (default) ==> f_ref == f_xo

Between the READY and LOCK there is a middle state LOCKWON:
    - Check, if it persists, there  is a hardware problem (and calib fails)
    - SRES takes it out

PA_POWER[8]
0x10: 0x01  -> 11dBm (max power)

0x18: 0x87 -> No ramping and 2.4pF (for high power possibly)

MOD1 (data rate)
0x1A: 0x93 -> Mantissa 0x93
0x1B: 0x1A -> Expo 0xA
-> Datarate of 25000000*(256+0x93)*2**0xA/(2**28)
== 38433.0749512

CHFLT channel filter bandwidth
0x1D: 0x13 ->  98 * 50000000/26000000

AFC2 Auto Freq Comp
0x1E: 0xC8 -> AFC_FREEZE_ON_SYNC : ON!

AGCCTRL
0x25: 0x62  -> Thresh low, lower!

ANT_SEL_CONF (parece no necesario) (para switchear antenas)
0x27: 0x15  -> CS_BLANCKING

PCKTCTRL:
0x32: 0x3F: -> Increase preamble length
0x33: 0x30: -> Whitening enable, CRC 0x07

packet length lsb
PCKTLEN0:
0x35: 0x12

Packet filter opts
0x4F: 0x41 -> discard if CRC bad, RXtimeout or'd from CS/SQI/PQI masks

Protocol
0x50: 0x40 -> SQI timeout mask
0x51: 0x01 -> Auto pckt filtering

---- Don't do this -----
RCO_VCO_CALIBR_IN:
0x6E: 40 -> for TX
0x6F: 41 -> for RX
----------------------

VCO_CONF
0xA1: 0x25 -> vco current increased. Device ERRATA!

write:
0xBC <= 0x22
during radio configuration to avoid data corruption (with calib disable) Errata

PM_CONFIG
0xA4: 0x0C SET SMPS Vtune voltage and bandwith

Leer cosas de los readonly registers

------------- 
mi prueba: Leer calib out antes de calibración:

Calib out
Address: 0xE5, Value: 0x00
Address: 0xE4, Value: 0x70


----
Ejecutar calibración:
0x50: 0x46

--------
Calib out, after calibration
Address: 0xE5, Value: 0x80
Address: 0xE4, Value: 0x65


-------
Write data to send!
0xff: 0x66
0xff: 0x72
0xff: 0x75


------
Transmit!!!
CMD: 0x60




Calib in
Address: 0x6F, Value: 0x48
Address: 0x6E, Value: 0x48

Sequence
Address: 0xC2, Value: 0x00


---------

---------
This is not necessary for reception
For RX:
PA_POWER[8]
0x18: 0x07 -> No ramping and 0pF

RSSI Threshold
0x22: 0x00

RX timeout presc and counter
0x53: 0x09
0x54: 0xCE

PM_Config (SMPS)
0xA5: 0x98

AFC_CORR:
0xC4: 0xFE

Link_Qualif:
0xC5: 0x10
0xC6: 0x18
0xC7: 0x28
0xC8: 0x26



---------------
For simple STack packet:

0x31: 0xC7  Stack packet selected
0x30: 0x10  Address_len = 2



-----------
For auto Ack
TX:
    0x52: 0x00 //Disable NACK_TX
RX:
    0x52: 0x0C // Enable NACK_TX and enable AUTO_ACK
    0x51: 0x41 // Piggybacking on
    //Write TX FIFO piggyback response data
    RX command (wait)
TX:
    0xFF: Write data
    TX command
    read from 0xFF the piggybacked data
