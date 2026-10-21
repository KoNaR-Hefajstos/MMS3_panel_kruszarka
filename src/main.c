#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>

#include "iG5A.h"
#include "zephyr/sleep.h"


LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define CHAINBUS_LED_COUNT 5 
#define CHAINBUS_BUTTON_COUNT 5 


const static struct gpio_dt_spec chainbus_led[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_led4), gpios),
};

const static struct gpio_dt_spec chainbus_button[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button3), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(chainbus_button4), gpios),
};


#define CHAINBUS_NODE DT_NODELABEL(chainbus0)

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

#define MIN_FREQUENCY 10.0f
#define MAX_FREQUENCY 50.0f
#define FREQUENCY_STEP 1.0f
#define FREQUENCY_REPEAT_MS 300.0f

int frequency = 10;
int64_t last_frequency_change = 0;



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
const static struct gpio_dt_spec expander_pinouts[] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart1_oe),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart1_rs485_re_n),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart1_shdn_n),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart1_bonus_out0),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart1_bonus_out1),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(status_led),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart2_oe),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart2_rs485_re_n),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart2_shdn_n),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart2_bonus_out0),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(uart2_bonus_out1),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(spi1_enable),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(spi1_bonus_in0),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(spi1_bonus_in1),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(spi1_bonus_in2),gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(spi1_bonus_out),gpios)
};

#define EXPANDER_PINOUTS_COUNT 16

const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(slot0_uart));


iG5A inverter;


int init();
void operatePanel();
void increase_frequency(void);
void decrease_frequency(void);
void crusher_stop(void);
void crusher_rotate_left(void);
void crusher_rotate_right(void);

int main(void)
{
    if(init()){
        LOG_ERR("INIT FAILED!");
        return -1;
    }


    while (1) {

        operatePanel();
    }

    return 0;
}


int init(){
    int ret; 

    for (int i = 0; i < 8; i++) {
        if (!device_is_ready(cs[i].port)) {
            return -ENODEV;
        }

        int ret = gpio_pin_configure_dt(&cs[i], GPIO_OUTPUT_INACTIVE);
        if (ret) {
            return ret;
        }
    }

    for (int i = 0; i < CHAINBUS_LED_COUNT; i++) {
        if (!device_is_ready(chainbus_led[i].port)) {
            LOG_ERR("Error: Chainbus LED device %s is not ready\n", chainbus_led[i].port->name);
            return 1;
        }
        ret = gpio_pin_configure_dt(&chainbus_led[i], GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Error %d: failed to configure Chainbus LED pin %d\n", ret, chainbus_led[i].pin);
            return 1;
        }
    }

    for(int i = 0; i< CHAINBUS_BUTTON_COUNT; i++){
        if (!gpio_is_ready_dt(&chainbus_button[i])) {
            LOG_ERR("GPIO not ready");
            return 1;
        }
    }

    for (int i = 0; i < CHAINBUS_LED_COUNT; i++) {
        gpio_pin_set_dt(&chainbus_led[i], 1);
        //gpio_pin_toggle_dt(&chainbus_led[i]);
    }




    for(int i = 0; i<=STATUS_LED; i++){

        if(!device_is_ready(expander_pinouts[i].port)){
           LOG_ERR("ERROR: expander pin %s device is not ready!",expander_pinouts[i].port->name) ;
           return 1;
        }
        
        ret = gpio_pin_configure_dt(&expander_pinouts[i],GPIO_OUTPUT_INACTIVE);
        if(ret){
            LOG_ERR("ERROR couldnt configure pin %s",expander_pinouts[i].port->name);
        }
    }


    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return 1;
    }



        //gpio_pin_set_dt(&expander_pinouts[UART1_OE], 1);
    gpio_pin_set_dt(&expander_pinouts[UART1_SHDN_N], 0);
    gpio_pin_set_dt(&expander_pinouts[UART1_RS485_RE_N], 1);
    //gpio_pin_set_dt(&expander_pinouts[UART2_BONUS_OUT0], 1);
    //gpio_pin_set_dt(&expander_pinouts[UART2_BONUS_OUT1], 1);


    //gpio_pin_set_dt(&expander_pinouts[UART2_OE], 1);
    //gpio_pin_set_dt(&expander_pinouts[UART1_SHDN_N], 0);
    //gpio_pin_set_dt(&expander_pinouts[UART1_RS485_RE_N], 1);
    //gpio_pin_set_dt(&expander_pinouts[UART2_BONUS_OUT0], 1);
    //gpio_pin_set_dt(&expander_pinouts[UART2_BONUS_OUT1], 1);



	iG5A_init(&inverter, 0x01);
	unlock(uart_dev,&inverter);
	//start(uart_dev, &inverter, true);
    stop(uart_dev,&inverter);
	set_frequency(uart_dev, &inverter, frequency);




    return 0;
}


void operatePanel(){
    int64_t now = k_uptime_get();
    int buttons[5] = {0,0,0,0,0};

        for(int i = 0; i < CHAINBUS_BUTTON_COUNT; i++){
            buttons[i] = gpio_pin_get_dt(&chainbus_button[i]);
            //LOG_INF("button %d= %d\n",i, buttons[i]);
        }


    if (now - last_frequency_change >= FREQUENCY_REPEAT_MS) {

        if (buttons[CRUSHER_STOP]) {
            crusher_stop();
        }else if (buttons[CRUSHER_SPEED_UP]) {
            increase_frequency();
            last_frequency_change = now;
        }else if (buttons[CRUSHER_SPEED_DOWN]) {
            decrease_frequency();
            last_frequency_change = now;
        } else if (buttons[CRUSHER_ROTATE_LEFT]) {
            crusher_rotate_left();
            last_frequency_change = now;
        }else if (buttons[CRUSHER_ROTATE_RIGHT]) {
            crusher_rotate_right();
            last_frequency_change = now;
        }
    }
}




void increase_frequency(void)
{
    if (frequency < MAX_FREQUENCY) {
        frequency += FREQUENCY_STEP;

        if (frequency > MAX_FREQUENCY) {
            frequency = MAX_FREQUENCY;
        }

	    set_frequency(uart_dev, &inverter, (float)frequency);
        LOG_INF("+ frequency: %d\n",frequency);
    }
}

void decrease_frequency(void)
{
    if (frequency > MIN_FREQUENCY) {
        frequency -= FREQUENCY_STEP;

        if (frequency < MIN_FREQUENCY) {
            frequency = MIN_FREQUENCY;
        }

	    set_frequency(uart_dev, &inverter, (float)frequency);
        LOG_INF("- frequency: %d\n",frequency);
    }
}




void crusher_stop(void){
    LOG_INF("STOP");
    stop(uart_dev,&inverter);

}
void crusher_rotate_left(void){
    LOG_INF("Rot left");
    stop(uart_dev,&inverter);
    k_sleep(K_SECONDS(10));
    start(uart_dev, &inverter, true);
}
void crusher_rotate_right(void){
    LOG_INF("ROT right"); // Crushing
    stop(uart_dev,&inverter);
    k_sleep(K_SECONDS(10));
    start(uart_dev, &inverter, false);

}