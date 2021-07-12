# steval-ids001v4m/spirit1 example

This example can be use to transmit and receive data

- Just flash the program in both transmitter and receiver
- You need to connect both steval boards to a USB port in some computer
- With minicom or a similar terminal:
  - In receiver use command receive ("c" and then "0x61").
    - Check with command "s" that the radio is in RX state
  - In transmitter use command "t", then write the text you want to transmit
    and press enter. Data is transmitted and automatically received on receiver
  - Receiver uses an interrupt to read data out of the buffer automatically
    and gets printed on console.
- This example uses:
  - Basic protocol with address length "1".
  - It does destination-based address filtering
  - GFSK modulation
  - Many things can be adjusted to taste




