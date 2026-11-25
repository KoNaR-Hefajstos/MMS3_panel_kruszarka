#include "panel.h"

LOG_MODULE_REGISTER(panel, LOG_LEVEL_INF);


static int raw_button_state[CHAINBUS_BUTTON_COUNT] = {0};
static int stable_button_state[CHAINBUS_BUTTON_COUNT] = {0};
static int64_t button_change_time[CHAINBUS_BUTTON_COUNT] = {0};



static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(slot0_uart));

static const struct gpio_dt_spec expander_pinouts[] = {
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

int init_panel(){
    int ret = 0; 

    for (int i = 0; i < 8; i++) {
        if (!device_is_ready(cs[i].port)) {
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&cs[i], GPIO_OUTPUT_INACTIVE);
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
    //gpio_pin_set_dt(&expander_pinouts[UART2_SHDN_N], 0);
    //gpio_pin_set_dt(&expander_pinouts[UART2_RS485_RE_N], 1);
    //gpio_pin_set_dt(&expander_pinouts[UART2_BONUS_OUT0], 1);
    //gpio_pin_set_dt(&expander_pinouts[UART2_BONUS_OUT1], 1);

    return ret;
}
void operatePanel(void)
{
    int buttons[CHAINBUS_BUTTON_COUNT] = {0};

    read_buttons_debounced(buttons);

    if (buttons[CRUSHER_STOP]) {
        crusher_send_command(CRUSHER_CMD_STOP);
        return;
    }

    if (buttons[CRUSHER_SPEED_UP]) {
        crusher_send_command(CRUSHER_CMD_SPEED_UP);
    }
    else if (buttons[CRUSHER_SPEED_DOWN]) {
        crusher_send_command(CRUSHER_CMD_SPEED_DOWN);
    }
    else if (buttons[CRUSHER_ROTATE_LEFT]) {
        crusher_send_command(CRUSHER_CMD_ROTATE_LEFT);
    }
    else if (buttons[CRUSHER_ROTATE_RIGHT]) {
        crusher_send_command(CRUSHER_CMD_ROTATE_RIGHT);
    }
}

void panel_thread(void *arg1, void *arg2, void *arg3)
{
    while (1) {
        operatePanel();
        k_sleep(K_MSEC(100));
    }
}


K_THREAD_DEFINE(
    panel_tid,
    PANEL_STACK_SIZE,
    panel_thread,
    NULL,
    NULL,
    NULL,
    PANEL_PRIORITY,
    0,
    K_TICKS_FOREVER
);

void panel_start_thread(){
    k_thread_start(panel_tid);
}


void read_buttons_debounced(int buttons[])
{
    int64_t now = k_uptime_get();

    for (int i = 0; i < CHAINBUS_BUTTON_COUNT; i++) {

        int raw = gpio_pin_get_dt(&chainbus_button[i]);

        if (raw < 0) {
            LOG_ERR("GPIO read error: button %d, error %d", i, raw);
            buttons[i] = 0;
            continue;
        }

        if (raw != raw_button_state[i]) {
            raw_button_state[i] = raw;
            button_change_time[i] = now;
        }

        if ((now - button_change_time[i]) >= DEBOUNCE_MS) {
            stable_button_state[i] = raw_button_state[i];
        }

        buttons[i] = stable_button_state[i];
    }
}
