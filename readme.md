# MMS3 Panel Crusher 

This application controls a crusher using an operator push-button panel.

The stackup is as such:
0. **MMS3 STM32F405**
1. **Slot 0 Hat Protocol** https://github.com/KoNaR-Hefajstos/MMS3_hat_protocol
2. **Slot 1 Hat GPIO Baisc**: https://github.com/KoNaR-Hefajstos/MMS3_hat_GPIO_board_basic


## Hardware Overview

### MMS3 Baseboard
- **MCU**: STM32F405
- **Physical Buses Used**:
  - `I2C2` (`chainbus_i2c`)
  - `USART3` (`chainbus_usart`) operating at 9600 baud

### Connected HAT Modules

#### Slot 0: Protocol HAT (`hat0@0`)
- **Virtual I2C Bridge**: `slot0_i2c` connected to `&i2c2`
- **GPIO Expander**: PCA9555 at address `0x20`
- **Pin Assignment**:
  - `P00`–`P04`: UART1 controls (`oe`, `rs485_re_n`, `shdn_n`, `bonus_out0`, `bonus_out1`)
  - `P05`: Status LED (`status_led`)
  - `P06`–`P10`: UART2 controls (`oe`, `rs485_re_n`, `shdn_n`, `bonus_out0`, `bonus_out1`)
  - `P11`–`P15`: SPI1 controls (`spi1_enable`, `bonus_in0..2`, `bonus_out`)
- **Virtual UART Bridge**: `slot0_uart` mapping `&usart3` at 9600 baud

#### Slot 1: Expander HAT (`hat1@1`)
- **Virtual I2C Bridge**: `slot1_i2c` connected to `&i2c2`
- **GPIO Expander**: PCA9555 at address `0x20`
- **Peripherals**:
  - **5x User LEDs**: Connected to expander pins 1, 5, 7, 9, 11
  - **5x User Buttons**: Connected to expander pins 0, 4, 6, 8, 10 (Active Low, internal pull-up enabled)

---
### Connecting to panel

- **Speed up**:             panel = A; GPIO:  button = PIN5,  led = PIN6
- **Rotation right**:       panel = B; GPIO:  button = PIN1,  led = PIN2
- **Stop**:                 panel = C; GPIO:  button = PIN11, led = PIN12
- **Rotation left**:        panel = D; GPIO:  button = PIN7,  led = PIN8
- **Speed down**:           panel = E; GPIO:  button = PIN9,  led = PIN10

### Connecting to hat protocol

Connect cable from inverter to UART1