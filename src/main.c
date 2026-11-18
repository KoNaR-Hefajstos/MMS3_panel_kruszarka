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

#include "drivers/inverter/ig5a.h"

#include "panel.h"
#include "crusher.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
int init();

int main(void)
{
    if(init())
    {
        LOG_ERR("INIT FAILED!");
        return -1;
    }


    return 0;
}


int init(){
    int ret; 
    if(init_panel()){
        return EINVAL;
    }


    if(init_crusher()){
        return 1;
    }

    crusher_start_thread();
    panel_start_thread();

    return 0;
}

