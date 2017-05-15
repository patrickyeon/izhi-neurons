#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "./izhi.h"

#define FLOATPIN GPIO1
#define FIXEDPIN GPIO3

int main(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    gpio_clear(GPIOA, GPIO15);
    gpio_clear(GPIOB, GPIO5);
    fneuron_t spiky_f;
    ineuron_t spiky_i;
    RS_f(&spiky_f);
    RS_i(&spiky_i);
    for (int i = 0; i < 5000; i++) {
        if (i < 100) {
            step_f(&spiky_f, 0, 0.1);
            step_i(&spiky_i, 0, 10);
        } else {
            gpio_set(GPIOA, GPIO15);
            step_f(&spiky_f, 10, 0.1);
            gpio_clear(GPIOA, GPIO15);
            
            gpio_set(GPIOB, GPIO5);
            step_i(&spiky_i, 10 * spiky_i.scale, 10);
            gpio_clear(GPIOB, GPIO5);
        }
    }
}
