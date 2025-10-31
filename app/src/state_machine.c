// HEADERS

#include <zephyr/smf.h>

#include "LED.h"
#include "BTN.h"
#include "state_machine.h"
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STANDBY_HOLD_TIME_MS 3000
#define PULSAR_MAX 1000


// TYPEDEFS

typedef enum {
    STATE_1,
    STATE_2,
    STATE_3,
    STANDBY
} project_machine_states;

typedef struct {
    // context
    struct smf_ctx ctx;

    uint16_t count;
    bool status_led_state;

    uint16_t standby_count;
    uint16_t pulsar;
    bool pulsar_reversal_state;
    project_machine_states last_state;
} state_object_t;


// FUNCTION PROTOTYPES

static void state_1_entry(void* o);
static enum smf_state_result state_1_run(void* o);

static void state_2_entry(void* o);
static enum smf_state_result state_2_run(void* o);

static void state_3_entry(void* o);
static enum smf_state_result state_3_run(void* o);

static void standby_entry(void* o);
static enum smf_state_result standby_run(void* o);

static void handle_standby_btn_press(project_machine_states current_state);
static void fresh_state_entry();


// LOCAL VARIABLES

static const struct smf_state project_states[] = {
    [STATE_1] = SMF_CREATE_STATE(state_1_entry, state_1_run, NULL, NULL, NULL),
    [STATE_2] = SMF_CREATE_STATE(state_2_entry, state_2_run, NULL, NULL, NULL),
    [STATE_3] = SMF_CREATE_STATE(state_3_entry, state_3_run, NULL, NULL, NULL),
    [STANDBY] = SMF_CREATE_STATE(standby_entry, standby_run, NULL, NULL, NULL),
};

static state_object_t state_object;


// FUNCTIONS

void state_machine_init() {
    state_object.count = 0;
    state_object.status_led_state = false;
    state_object.pulsar = 0;
    state_object.pulsar_reversal_state = false;
    state_object.standby_count = 0;

    smf_set_initial(SMF_CTX(&state_object), &project_states[STATE_1]);
}

int state_machine_run() {
    return smf_run_state(SMF_CTX(&state_object));
}

static void fresh_state_entry() {
    state_object.count = 0;
    state_object.status_led_state = false;
    state_object.pulsar = 0;
    state_object.pulsar_reversal_state = false;
    state_object.standby_count = 0;

    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_OFF);
}

static void state_1_entry(void* o) {
    printk("Entering State 1 - LED at 1Hz\n");
    fresh_state_entry();
}

static enum smf_state_result state_1_run(void* o) {
    if (state_object.count > 500) {
        state_object.count = 0;
        state_object.status_led_state = !state_object.status_led_state;
        LED_set(LED3, state_object.status_led_state ? LED_ON : LED_OFF);
    } else {
        state_object.count++;
    }

    handle_standby_btn_press(STATE_1);

    return SMF_EVENT_HANDLED;
}

static void state_2_entry(void* o) {
    printk("Entering State 2 - LED at 4Hz\n");
    fresh_state_entry();
}

static enum smf_state_result state_2_run(void* o) {
    if (state_object.count > 125) {
        state_object.count = 0;
        state_object.status_led_state = !state_object.status_led_state;
        LED_set(LED3, state_object.status_led_state ? LED_ON : LED_OFF);
    } else {
        state_object.count++;
    }

    handle_standby_btn_press(STATE_2);

    return SMF_EVENT_HANDLED;
}

static void state_3_entry(void* o) {
    printk("Entering State 3 - LED at 16Hz\n");
    fresh_state_entry();
}

static enum smf_state_result state_3_run(void* o) {
    if (state_object.count > 31) {
        state_object.count = 0;
        state_object.status_led_state = !state_object.status_led_state;
        LED_set(LED3, state_object.status_led_state ? LED_ON : LED_OFF);
    } else {
        state_object.count++;
    }

    handle_standby_btn_press(STATE_3);

    return SMF_EVENT_HANDLED;
}

static void handle_standby_btn_press(project_machine_states current_state) {
    if (BTN_is_pressed(BTN0) && BTN_is_pressed(BTN1)) {
        if (state_object.standby_count >= STANDBY_HOLD_TIME_MS) {
            state_object.standby_count = 0;
            state_object.last_state = current_state;
            smf_set_state(SMF_CTX(&state_object), &project_states[STANDBY]);
        } else {
            state_object.standby_count++;
        }
    } else {
        state_object.standby_count = 0;
    }
}

static void standby_entry(void* o) {
    printk("Entering Standby State - LED PULSE\n");
    fresh_state_entry();

    // reset button press state so we don't immediately exit standby
    BTN_check_clear_pressed(BTN0);
    BTN_check_clear_pressed(BTN1);
}

static enum smf_state_result standby_run(void* o) {
    if (state_object.pulsar >= PULSAR_MAX) {
        state_object.pulsar_reversal_state = true;
    } else if (state_object.pulsar == 0) {
        state_object.pulsar_reversal_state = false;
    }

    if (state_object.pulsar_reversal_state) {
        state_object.pulsar--;
    } else {
        state_object.pulsar++;
    }

    int brightness = (state_object.pulsar * 100 / PULSAR_MAX);

    LED_pwm(LED0, brightness);
    LED_pwm(LED1, brightness);
    LED_pwm(LED2, brightness);
    LED_pwm(LED3, brightness);

    if (BTN_check_clear_pressed(BTN0) || BTN_check_clear_pressed(BTN1) || BTN_check_clear_pressed(BTN2) || BTN_check_clear_pressed(BTN3)) {
        smf_set_state(SMF_CTX(&state_object), &project_states[state_object.last_state]);
    }

    return SMF_EVENT_HANDLED;
}

// static void led_on_state_entry(void* o) {
//     LED_set(LED3, LED_ON);
// }

// static enum smf_state_result led_on_state_run(void* o) {
//     if (led_state_object.count > 500) {
//         led_state_object.count = 0;
//         smf_set_state(SMF_CTX(&led_state_object), &led_states[LED_OFF_STATE]);
//     } else {
//         led_state_object.count++;
//     }

//     return SMF_EVENT_HANDLED;
// }

// static void led_off_state_entry(void* o) {
//     LED_set(LED3, LED_OFF);
// }

// static enum smf_state_result led_off_state_run(void* o) {
//     if (led_state_object.count > 500) {
//         led_state_object.count = 0;
//         smf_set_state(SMF_CTX(&led_state_object), &led_states[LED_ON_STATE]);
//     } else {
//         led_state_object.count++;
//     }

//     return SMF_EVENT_HANDLED;
// }