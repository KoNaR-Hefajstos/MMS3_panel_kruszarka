#ifndef PANEL_H
#define PANEL_H
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>

#include "zephyr/sleep.h"

#include "crusher.h"


#define PANEL_STACK_SIZE 1024
#define PANEL_PRIORITY 5

#define CHAINBUS_LED_COUNT 5 
#define CHAINBUS_BUTTON_COUNT 5 

#define EXPANDER_PINOUTS_COUNT 16

#define DEBOUNCE_MS 50

#define CHAINBUS_NODE DT_NODELABEL(chainbus0)





static const struct gpio_dt_spec chainbus_led[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led4), gpios),
};

static const struct gpio_dt_spec chainbus_button[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button4), gpios),
};




static const struct gpio_dt_spec cs[8] = {
    GPIO_DT_SPEC_GET_BY_IDX(CHAINBUS_NODE, cs_gpios, 0),
    GPIO_DT_SPEC_GET_BY_IDX(CHAINBUS_NODE, cs_gpios, 1),
    GPIO_DT_SPEC_GET_BY_IDX(CHAINBUS_NODE, cs_gpios, 2),
    GPIO_DT_SPEC_GET_BY_IDX(CHAINBUS_NODE, cs_gpios, 3),
    GPIO_DT_SPEC_GET_BY_IDX(CHAINBUS_NODE, cs_gpios, 4),
    GPIO_DT_SPEC_GET_BY_IDX(CHAINBUS_NODE, cs_gpios, 5),
    GPIO_DT_SPEC_GET_BY_IDX(CHAINBUS_NODE, cs_gpios, 6),
    GPIO_DT_SPEC_GET_BY_IDX(CHAINBUS_NODE, cs_gpios, 7),
};



enum PanelPins{
    CRUSHER_ROTATE_RIGHT,
    CRUSHER_SPEED_UP,
    CRUSHER_ROTATE_LEFT,
    CRUSHER_SPEED_DOWN,
    CRUSHER_STOP,
    
};





enum ExpanderPinouts{
    UART1_OE = 0,
    UART1_RS485_RE_N,
    UART1_SHDN_N,
    UART1_BONUS_OUT0,
    UART1_BONUS_OUT1,
    STATUS_LED,
    UART2_OE,
    UART2_RS485_RE_N,
    UART2_SHDN_N,
    UART2_BONUS_OUT0,
    UART2_BONUS_OUT1,
    SPI1_ENABLE,
    SPI1_BONUS_IN0,
    SPI1_BONUS_IN1,
    SPI1_BONUS_IN2,
    SPI1_BONUS_OUT
};





int init_panel();
void panel_start_thread();
void panel_thread(void *arg1, void *arg2, void *arg3);

void operatePanel();
void read_buttons_debounced(int buttons[]);

#endif