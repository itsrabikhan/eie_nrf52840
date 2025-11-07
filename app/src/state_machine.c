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
    INPUT_STATE,
    MENU_STATE,
    SAVED_STATE,
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

    uint8_t buffer;
    int buffer_index;

    char ascii_string[64];
    uint8_t string_index;
} state_object_t;


// FUNCTION PROTOTYPES

static void input_state_entry(void* o);
static enum smf_state_result input_state_run(void* o);

static void menu_state_entry(void* o);
static enum smf_state_result menu_state_run(void* o);

static void saved_state_entry(void* o);
static enum smf_state_result saved_state_run(void* o);

static void standby_entry(void* o);
static enum smf_state_result standby_run(void* o);

static void handle_standby_btn_press(project_machine_states current_state);
static void fresh_state_entry();
static void blink_led(int frequency_hz);


// LOCAL VARIABLES

static const struct smf_state project_states[] = {
    [INPUT_STATE] = SMF_CREATE_STATE(input_state_entry, input_state_run, NULL, NULL, NULL),
    [MENU_STATE] = SMF_CREATE_STATE(menu_state_entry, menu_state_run, NULL, NULL, NULL),
    [SAVED_STATE] = SMF_CREATE_STATE(saved_state_entry, saved_state_run, NULL, NULL, NULL),
    [STANDBY] = SMF_CREATE_STATE(standby_entry, standby_run, NULL, NULL, NULL),
};

static state_object_t state_object;


// FUNCTIONS

static void blink_led(int frequency_hz) {
    int count = 1000 / (frequency_hz * 2);
    if (state_object.count > count) {
        state_object.count = 0;
        state_object.status_led_state = !state_object.status_led_state;
        LED_set(LED3, state_object.status_led_state ? LED_ON : LED_OFF);
    } else {
        state_object.count++;
    }
}

void state_machine_init() {
    state_object.count = 0;
    state_object.status_led_state = false;
    state_object.pulsar = 0;
    state_object.pulsar_reversal_state = false;
    state_object.standby_count = 0;

    smf_set_initial(SMF_CTX(&state_object), &project_states[INPUT_STATE]);
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

static void input_state_entry(void* o) {
    printk("Entering Input State - LED at 1Hz\n");
    fresh_state_entry();
}

static enum smf_state_result input_state_run(void* o) {
    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    blink_led(1);

    // reflect button states on LEDs
    LED_set(LED0, BTN_is_pressed(BTN0) ? LED_ON : LED_OFF);
    LED_set(LED1, BTN_is_pressed(BTN1) ? LED_ON : LED_OFF);

    int index = state_object.buffer_index;
    if (BTN_check_clear_pressed(BTN0)) {
        // set bit at index position to 0
        state_object.buffer &= ~(1u << (7 - index));
        state_object.buffer_index = index + 1;
        printk("User entered 0 - Buffer ASCII value: %u\n", state_object.buffer);
    } else if (BTN_check_clear_pressed(BTN1)) {
        // set bit at index position to 1
        state_object.buffer |= (1u << (7 - index));
        state_object.buffer_index = index + 1;
        printk("User entered 1 - Buffer ASCII value: %u\n", state_object.buffer);
    } else if (BTN_check_clear_pressed(BTN2)) {
        // reset buffer and index
        state_object.buffer = 0;
        index = 0;
        printk("User cleared buffer\n");
    } else if (BTN_check_clear_pressed(BTN3)) {
        // save code as is
        index = 8;
    }

    if (index >= 8) {
        index = 0;
        state_object.buffer_index = 0;
        // convert buffer to ASCII character
        char ascii_char = (char) (state_object.buffer);
        state_object.ascii_string[state_object.string_index] = ascii_char;
        state_object.string_index++;
        state_object.ascii_string[state_object.string_index] = '\0';
        printk("Saved character: %c, Current string: %s\n", ascii_char, state_object.ascii_string);
        state_object.buffer = 0;
        smf_set_state(SMF_CTX(&state_object), &project_states[MENU_STATE]);
        return SMF_EVENT_HANDLED;
    }

    handle_standby_btn_press(INPUT_STATE);

    return SMF_EVENT_HANDLED;
}

static void menu_state_entry(void* o) {
    printk("Entering Menu State - LED at 4Hz\n");
    fresh_state_entry();
}

static enum smf_state_result menu_state_run(void* o) {
    blink_led(4);

    // if BTN0 or BTN1 pressed, go back to input state
    // the button that is pressed will be reflected in the input state
    // unless somehow they press and release in <1ms
    if (BTN_check_pressed(BTN0) || BTN_check_pressed(BTN1)) {
        smf_set_state(SMF_CTX(&state_object), &project_states[INPUT_STATE]);
        return SMF_EVENT_HANDLED;
    }

    if (BTN_check_clear_pressed(BTN2)) {
        state_object.string_index = 0;
        state_object.ascii_string[0] = '\0';
        printk("Cleared saved string.\n");
        smf_set_state(SMF_CTX(&state_object), &project_states[INPUT_STATE]);
        return SMF_EVENT_HANDLED;
    }

    if (BTN_check_clear_pressed(BTN3)) {
        printk("Saved string: %s\n", state_object.ascii_string);
        smf_set_state(SMF_CTX(&state_object), &project_states[SAVED_STATE]);
        return SMF_EVENT_HANDLED;
    }

    handle_standby_btn_press(MENU_STATE);

    return SMF_EVENT_HANDLED;
}

static void saved_state_entry(void* o) {
    printk("Entering Saved String State - LED at 16Hz\n");
    fresh_state_entry();
}

static enum smf_state_result saved_state_run(void* o) {
    blink_led(16);

    if (BTN_check_clear_pressed(BTN2)) {
        state_object.string_index = 0;
        state_object.ascii_string[0] = '\0';
        printk("Cleared saved string.\n");
        smf_set_state(SMF_CTX(&state_object), &project_states[INPUT_STATE]);
        return SMF_EVENT_HANDLED;
    }

    if (BTN_check_clear_pressed(BTN3)) {
        printk("Saved string: %s\n", state_object.ascii_string);
        return SMF_EVENT_HANDLED;
    }

    handle_standby_btn_press(SAVED_STATE);

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

    state_object.buffer = 0;
    state_object.buffer_index = 0;

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