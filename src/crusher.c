#include "crusher.h"


LOG_MODULE_REGISTER(crusher, LOG_LEVEL_INF);


#define MIN_FREQUENCY 10.0f
#define MAX_FREQUENCY 50.0f
#define FREQUENCY_STEP 1.0f
#define FREQUENCY_REPEAT_MS 300.0f

static int frequency = 10;
static const struct device *inverter = DEVICE_DT_GET(DT_NODELABEL(ig5a0));
static enum CrusherMovement crusherMovement = CRUSHER_MOVEMENT_STOP; 

#define CRUSHER_QUEUE_SIZE 10

#define CRUSHER_STACK_SIZE 2048
#define CRUSHER_PRIORITY 5

K_MSGQ_DEFINE(crusher_msgq,
              sizeof(enum CrusherCommand),
              CRUSHER_QUEUE_SIZE,
              4);



int init_crusher(){
    int ret = 0;
    if (!device_is_ready(inverter)) {
        LOG_ERR("iG5A: device not ready\n");
        return 1;
    }

    ig5a_unlock(inverter);
    ig5a_stop(inverter);
    ig5a_set_frequency(inverter, frequency);

    return ret;
}

int crusher_send_command(enum CrusherCommand command)
{
    return k_msgq_put(&crusher_msgq, &command, K_NO_WAIT);
}


void crusher_thread(void *arg1, void *arg2, void *arg3)
{
    enum CrusherCommand command;

    while (1) {
        k_msgq_get(&crusher_msgq, &command, K_FOREVER);

        switch (command) {
        case CRUSHER_CMD_STOP:
            crusher_stop();
            break;

        case CRUSHER_CMD_ROTATE_LEFT:
            crusher_rotate_left();
            break;

        case CRUSHER_CMD_ROTATE_RIGHT:
            crusher_rotate_right();
            break;

        case CRUSHER_CMD_SPEED_UP:
            crusher_increase_frequency();
            break;

        case CRUSHER_CMD_SPEED_DOWN:
            crusher_decrease_frequency();
            break;
        }
    }
}

K_THREAD_DEFINE(
    crusher_tid,
    CRUSHER_STACK_SIZE,
    crusher_thread,
    NULL,
    NULL,
    NULL,
    CRUSHER_PRIORITY,
    0,
    K_TICKS_FOREVER
);


void crusher_start_thread(void)
{
    k_thread_start(crusher_tid);
}



void crusher_increase_frequency(void)
{
    if (frequency < MAX_FREQUENCY) {
        frequency += FREQUENCY_STEP;

        if (frequency > MAX_FREQUENCY) {
            frequency = MAX_FREQUENCY;
        }

        ig5a_set_frequency(inverter, (float)frequency);
        LOG_INF("+ frequency: %d",frequency);
    }
}

void crusher_decrease_frequency(void)
{
    if (frequency > MIN_FREQUENCY) {
        frequency -= FREQUENCY_STEP;

        if (frequency < MIN_FREQUENCY) {
            frequency = MIN_FREQUENCY;
        }

        ig5a_set_frequency(inverter, (float)frequency);
        LOG_INF("- frequency: %d",frequency);
    }
}


void crusher_stop_when_rotating(void){
    ig5a_stop(inverter);
    if(frequency <= 10){
        k_sleep(K_SECONDS(3));
        return;
    }
    if(frequency <= 20){
        k_sleep(K_SECONDS(5));
        return;
    }
    if(frequency <= 30){
        k_sleep(K_SECONDS(7));
        return;
    }
    if(frequency <= 40){
        k_sleep(K_SECONDS(8));
        return;
    }
    if(frequency <= 50){
        k_sleep(K_SECONDS(10));
        return;
    }
}


void crusher_stop(void){
    LOG_INF("STOP");
    ig5a_stop(inverter);
    crusherMovement = CRUSHER_MOVEMENT_STOP;
}
void crusher_rotate_left(void){
    LOG_INF("Rot left");
    switch (crusherMovement) {
        case CRUSHER_MOVEMENT_STOP:
            ig5a_start(inverter, true);
            crusherMovement = CRUSHER_MOVEMENT_LEFT;
        break;
        case CRUSHER_MOVEMENT_RIGHT:
            crusher_stop_when_rotating();
            ig5a_start(inverter, true);
            crusherMovement = CRUSHER_MOVEMENT_LEFT;
        break;
        case CRUSHER_MOVEMENT_LEFT:
            crusherMovement = CRUSHER_MOVEMENT_LEFT;
        break;
        default:
            LOG_ERR("Wrong crusherMovement");
            ig5a_stop(inverter);
            crusherMovement = CRUSHER_MOVEMENT_STOP;
        break;
    }
}
void crusher_rotate_right(void){
    LOG_INF("ROT right"); // Crushing
    switch (crusherMovement) {
        case CRUSHER_MOVEMENT_STOP:
            ig5a_start(inverter, false);
            crusherMovement = CRUSHER_MOVEMENT_RIGHT;
        break;
        case CRUSHER_MOVEMENT_LEFT:
            crusher_stop_when_rotating();
            ig5a_start(inverter, false);
            crusherMovement = CRUSHER_MOVEMENT_RIGHT;
        break;
        case CRUSHER_MOVEMENT_RIGHT:
            crusherMovement = CRUSHER_MOVEMENT_RIGHT;
        break;
        default:
            LOG_ERR("Wrong crusherMovement");
            ig5a_stop(inverter);
            crusherMovement = CRUSHER_MOVEMENT_STOP;
        break;
    }
}




