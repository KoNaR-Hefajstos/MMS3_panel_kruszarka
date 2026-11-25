#ifndef CRUSHER_H 
#define CRUSHER_H 
#include <stdatomic.h>
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


enum CrusherCommand {
    CRUSHER_CMD_STOP,
    CRUSHER_CMD_ROTATE_LEFT,
    CRUSHER_CMD_ROTATE_RIGHT,
    CRUSHER_CMD_SPEED_UP,
    CRUSHER_CMD_SPEED_DOWN
};


enum CrusherMovement{
    CRUSHER_MOVEMENT_STOP,
    CRUSHER_MOVEMENT_LEFT,
    CRUSHER_MOVEMENT_RIGHT
};




int init_crusher();
int crusher_send_command(enum CrusherCommand command);
void crusher_start_thread(void);
void crusher_thread(void *arg1, void *arg2, void *arg3);

void crusher_increase_frequency(void);
void crusher_decrease_frequency(void);
void crusher_stop_when_rotating(void);
void crusher_stop(void);
void crusher_rotate_left(void);
void crusher_rotate_right(void);

#endif