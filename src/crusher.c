#include "crusher.h"


LOG_MODULE_REGISTER(crusher, LOG_LEVEL_INF);


#define MIN_FREQUENCY 10.0f
#define MAX_FREQUENCY 50.0f
#define FREQUENCY_STEP 1.0f
#define FREQUENCY_REPEAT_MS 300.0f

static int frequency = 10;
static const struct device *inverter = DEVICE_DT_GET(DT_NODELABEL(ig5a0));
static enum CrusherMovement crusherMovement = CRUSHER_MOVEMENT_STOP; 

#define CRUSHER_STACK_SIZE 2048
#define CRUSHER_PRIORITY 5

#define INVERTER_TX_INTERVAL_MS 200
#define STOP_POLL_MS 10

#define CRUSHER_CMD_NONE ((int)-1)

static atomic_int pending_command = CRUSHER_CMD_NONE;
static atomic_int stop_requested;

K_SEM_DEFINE(crusher_cmd_sem, 0, 1);

static int64_t last_inverter_tx_ms;
static bool inverter_tx_seen;


static int command_priority(int command)
{
    if (command == CRUSHER_CMD_NONE) {
        return 0;
    }

    if (command == CRUSHER_CMD_STOP) {
        return 3;
    }

    if (command == CRUSHER_CMD_ROTATE_LEFT ||
        command == CRUSHER_CMD_ROTATE_RIGHT) {
        return 2;
    }

    return 1;
}

static void inverter_tx_mark(void)
{
    last_inverter_tx_ms = k_uptime_get();
    inverter_tx_seen = true;
}

static void inverter_tx_now_unlock(void)
{
    (void)ig5a_unlock(inverter);
    inverter_tx_mark();
}

static void inverter_tx_now_stop(void)
{
    (void)ig5a_stop(inverter);
    inverter_tx_mark();
}

static void inverter_tx_now_start(bool reverse)
{
    (void)ig5a_start(inverter, reverse);
    inverter_tx_mark();
}

/* Zwraca false, gdy STOP przerwał oczekiwanie - ramka nie jest wysyłana. */
static bool inverter_tx_limited_freq(int target_frequency)
{
    if (inverter_tx_seen) {
        int64_t deadline = last_inverter_tx_ms + INVERTER_TX_INTERVAL_MS;

        while (k_uptime_get() < deadline) {
            if (atomic_load(&stop_requested) != 0) {
                return false;
            }

            k_sleep(K_MSEC(STOP_POLL_MS));
        }
    }

    (void)ig5a_set_frequency(inverter, (float)target_frequency);
    inverter_tx_mark();
    return true;
}


int init_crusher(){
    int ret = 0;
    if (!device_is_ready(inverter)) {
        LOG_ERR("iG5A: device not ready\n");
        return 1;
    }

    inverter_tx_now_unlock();
    inverter_tx_now_stop();
    (void)inverter_tx_limited_freq(frequency);

    return ret;
}

int crusher_send_command(enum CrusherCommand command)
{
    int expected = atomic_load(&pending_command);

    for (;;) {
        if (command_priority(command) < command_priority(expected)) {
            return 0;
        }

        if (atomic_compare_exchange_weak(&pending_command,
                                         &expected,
                                         (int)command)) {
            break;
        }
    }

    if (command == CRUSHER_CMD_STOP) {
        atomic_store(&stop_requested, 1);
    }

    k_sem_give(&crusher_cmd_sem);
    return 0;
}


void crusher_thread(void *arg1, void *arg2, void *arg3)
{
    int raw;

    while (1) {
        k_sem_take(&crusher_cmd_sem, K_FOREVER);

        raw = atomic_exchange(&pending_command, CRUSHER_CMD_NONE);
        if (raw == CRUSHER_CMD_NONE) {
            continue;
        }

        if (raw == CRUSHER_CMD_STOP) {
            atomic_store(&stop_requested, 0);
        }

        switch ((enum CrusherCommand)raw) {
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
        int next = frequency + (int)FREQUENCY_STEP;

        if (next > (int)MAX_FREQUENCY) {
            next = (int)MAX_FREQUENCY;
        }

        if (!inverter_tx_limited_freq(next)) {
            return;
        }

        frequency = next;
        LOG_INF("+ frequency: %d",frequency);
    }
}

void crusher_decrease_frequency(void)
{
    if (frequency > MIN_FREQUENCY) {
        int next = frequency - (int)FREQUENCY_STEP;

        if (next < (int)MIN_FREQUENCY) {
            next = (int)MIN_FREQUENCY;
        }

        if (!inverter_tx_limited_freq(next)) {
            return;
        }

        frequency = next;
        LOG_INF("- frequency: %d",frequency);
    }
}


void crusher_stop_when_rotating(void){
    int64_t delay_ms;
    int64_t deadline;

    inverter_tx_now_stop();

    if(frequency <= 10){
        delay_ms = 3000;
    } else if(frequency <= 20){
        delay_ms = 5000;
    } else if(frequency <= 30){
        delay_ms = 7000;
    } else if(frequency <= 40){
        delay_ms = 8000;
    } else if(frequency <= 50){
        delay_ms = 10000;
    } else {
        return;
    }

    deadline = k_uptime_get() + delay_ms;
    while (k_uptime_get() < deadline) {
        if (atomic_load(&stop_requested) != 0) {
            return;
        }

        k_sleep(K_MSEC(STOP_POLL_MS));
    }
}


void crusher_stop(void){
    if (crusherMovement == CRUSHER_MOVEMENT_STOP) {
        return;
    }

    LOG_INF("STOP");
    inverter_tx_now_stop();
    crusherMovement = CRUSHER_MOVEMENT_STOP;
}
void crusher_rotate_left(void){
    LOG_INF("Rot left");
    switch (crusherMovement) {
        case CRUSHER_MOVEMENT_STOP:
            inverter_tx_now_start(true);
            crusherMovement = CRUSHER_MOVEMENT_LEFT;
        break;
        case CRUSHER_MOVEMENT_RIGHT:
            crusher_stop_when_rotating();
            if (atomic_load(&stop_requested) != 0) {
                crusherMovement = CRUSHER_MOVEMENT_STOP;
                break;
            }
            inverter_tx_now_start(true);
            crusherMovement = CRUSHER_MOVEMENT_LEFT;
        break;
        case CRUSHER_MOVEMENT_LEFT:
            crusherMovement = CRUSHER_MOVEMENT_LEFT;
        break;
        default:
            LOG_ERR("Wrong crusherMovement");
            inverter_tx_now_stop();
            crusherMovement = CRUSHER_MOVEMENT_STOP;
        break;
    }
}
void crusher_rotate_right(void){
    LOG_INF("ROT right"); // Crushing
    switch (crusherMovement) {
        case CRUSHER_MOVEMENT_STOP:
            inverter_tx_now_start(false);
            crusherMovement = CRUSHER_MOVEMENT_RIGHT;
        break;
        case CRUSHER_MOVEMENT_LEFT:
            crusher_stop_when_rotating();
            if (atomic_load(&stop_requested) != 0) {
                crusherMovement = CRUSHER_MOVEMENT_STOP;
                break;
            }
            inverter_tx_now_start(false);
            crusherMovement = CRUSHER_MOVEMENT_RIGHT;
        break;
        case CRUSHER_MOVEMENT_RIGHT:
            crusherMovement = CRUSHER_MOVEMENT_RIGHT;
        break;
        default:
            LOG_ERR("Wrong crusherMovement");
            inverter_tx_now_stop();
            crusherMovement = CRUSHER_MOVEMENT_STOP;
        break;
    }
}




