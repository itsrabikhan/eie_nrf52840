// HEADER FILES

#include <zephyr/smf.h>

#include "LED.h"
#include "BTN.h"
#include "state_machine.h"
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>


// FUNCTION PROTOTYPES

static void state_1_entry(void* o);
static enum smf_state_result state_1_run(void* o);

static void state_2_entry(void* o);
static enum smf_state_result state_2_run(void* o);

static void state_3_entry(void* o);
static enum smf_state_result state_3_run(void* o);


// TYPEDEFS

enum project_machine_states {
    STATE_1,
    STATE_2,
    STATE_3,
};

typedef struct {
    // context
    struct smf_ctx ctx;

    uint16_t count;
    bool status_led_state;

    
} state_object_t;


// LOCAL VARIABLES

static const struct smf_state project_states[] = {
    [STATE_1] = SMF_CREATE_STATE(state_1_entry, state_1_run, NULL, NULL, NULL),
    [STATE_2] = SMF_CREATE_STATE(state_2_entry, state_2_run, NULL, NULL, NULL),
    [STATE_3] = SMF_CREATE_STATE(state_3_entry, state_3_run, NULL, NULL, NULL),
};

static state_object_t state_object;


// FUNCTIONS

void state_machine_init() {
    state_object.count = 0;
    smf_set_initial(SMF_CTX(&state_object), &project_states[STATE_1]);
}

int state_machine_run() {
    return smf_run_state(SMF_CTX(&state_object));
}

static void state_1_entry(void* o) {
    printk("Entering State 1 - LED at 1Hz\n");
    state_object.count = 0;
    LED_set(LED3, LED_OFF);
    state_object.status_led_state = false;
}

static enum smf_state_result state_1_run(void* o) {
    if (state_object.count > 500) {
        state_object.count = 0;
        state_object.status_led_state = !state_object.status_led_state;
        LED_set(LED3, state_object.status_led_state ? LED_ON : LED_OFF);
    } else {
        state_object.count++;
    }

    return SMF_EVENT_HANDLED;
}

static void state_2_entry(void* o) {
    printk("Entering State 2 - LED at 4Hz\n");
    state_object.count = 0;
    LED_set(LED3, LED_OFF);
    state_object.status_led_state = false;
}

static enum smf_state_result state_2_run(void* o) {
    if (state_object.count > 125) {
        state_object.count = 0;
        state_object.status_led_state = !state_object.status_led_state;
        LED_set(LED3, state_object.status_led_state ? LED_ON : LED_OFF);
    } else {
        state_object.count++;
    }

    return SMF_EVENT_HANDLED;
}

static void state_3_entry(void* o) {
    printk("Entering State 3 - LED at 16Hz\n");
    state_object.count = 0;
    LED_set(LED3, LED_OFF);
    state_object.status_led_state = false;
}

static enum smf_state_result state_3_run(void* o) {
    if (state_object.count > 31) {
        state_object.count = 0;
        state_object.status_led_state = !state_object.status_led_state;
        LED_set(LED3, state_object.status_led_state ? LED_ON : LED_OFF);
    } else {
        state_object.count++;
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