/*
 * main.c
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#include "LED.h"
#include "BTN.h"

void on_unlock(void) {
    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_OFF);

    printk("Unlocked!\n");

    k_msleep(500);

    LED_set(LED0, LED_ON);
    k_msleep(250);
    LED_set(LED1, LED_ON);
    k_msleep(250);
    LED_set(LED2, LED_ON);
    k_msleep(250);
    LED_set(LED3, LED_ON);
    k_msleep(500);

    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_OFF);
}

void incorrect_code() {
    for (int i = 0; i < 3; i++) {
        LED_set(LED0, LED_OFF);
        LED_set(LED1, LED_OFF);
        LED_set(LED2, LED_OFF);
        LED_set(LED3, LED_OFF);

        k_msleep(500);
        
        LED_set(LED0, LED_ON);
        LED_set(LED1, LED_ON);
        LED_set(LED2, LED_ON);
        LED_set(LED3, LED_ON);

        k_msleep(500);
    }

    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_OFF);
}

void btn_press(int btn, int *input_code, int *input_index, int *correct_code, int *locked) {
    if (*input_index < 4) {
        input_code[*input_index] = btn;
        (*input_index)++;
        printk("Button %d pressed. Input code: ", btn);
        LED_set(LED0 + btn, LED_ON);

        for (int i = 0; i < *input_index; i++) {
            printk("%d ", input_code[i]);
        }
        printk("\n");

        if (*input_index == 4) {
            int correct = 1;
            for (int i = 0; i < 4; i++) {
                if (input_code[i] != correct_code[i]) {
                    correct = 0;
                    break;
                }
            }
            if (correct) {
                *locked = 0;
                on_unlock();
            } else {
                printk("Incorrect code. Try again.\n");
                incorrect_code();
            }

            for (int i = 0; i < 4; i++) {
                input_code[i] = -1;
            }
            *input_index = 0;
        }
    }
}

int main(void) {
    if (BTN_init() < 0) {
        printk("Error initializing button.\n");
        return -1;
    }

    if (LED_init() < 0) {
        printk("Error initializing LED.\n");
        return -1;
    }

    int correct_code[4] = {BTN1, BTN3, BTN0, BTN2};
    printk("Correct code is: ");
    for (int i = 0; i < 4; i++) {
        printk("%d ", correct_code[i] + 1);
    }
    printk("\n");
    int input_code[4] = {-1, -1, -1, -1};
    int input_index = 0;

    printk("Starting lock system...\n");
    int locked = 1;
    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_OFF);
    printk("Locked!\n");

    while (1) {
        if (locked) {
            if (BTN_check_clear_pressed(BTN0)) {
                btn_press(0, input_code, &input_index, correct_code, &locked);
            }
            if (BTN_check_clear_pressed(BTN1)) {
                btn_press(1, input_code, &input_index, correct_code, &locked);
            }
            if (BTN_check_clear_pressed(BTN2)) {
                btn_press(2, input_code, &input_index, correct_code, &locked);
            }
            if (BTN_check_clear_pressed(BTN3)) {
                btn_press(3, input_code, &input_index, correct_code, &locked);
            }
        } else {
            if (BTN_is_pressed(BTN0)) {
                LED_set(LED0, LED_ON);
            } else {
                LED_set(LED0, LED_OFF);
            }

            if (BTN_is_pressed(BTN1)) {
                LED_set(LED1, LED_ON);
            } else {
                LED_set(LED1, LED_OFF);
            }

            if (BTN_is_pressed(BTN2)) {
                LED_set(LED2, LED_ON);
            } else {
                LED_set(LED2, LED_OFF);
            }

            if (BTN_is_pressed(BTN3)) {
                LED_set(LED3, LED_ON);
            } else {
                LED_set(LED3, LED_OFF);
            }
        }

        k_msleep(100);
    }
    
    return 0;
}
