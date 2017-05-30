#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "./izhi.h"

#define FLOATPIN GPIO1
#define FIXEDPIN GPIO3

int main(void) {
    // use the fastest internal clock
    rcc_set_sysclk_source(RCC_HSI16);
    rcc_osc_on(RCC_HSI16);

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    gpio_clear(GPIOA, GPIO15);
    gpio_clear(GPIOB, GPIO5);

    ineuron_t spiky_i;
    RS_i(&spiky_i);

    for (int i = 0; i < 100; i++) {
        step_i(&spiky_i, 0, 10);
    }

    while(1) {
        // blip one pin so that I can calibrate out the time it takes to turn
        //  a pin on and off from the loop timing later.
        gpio_set(GPIOA, GPIO15);
        gpio_clear(GPIOA, GPIO15);

        gpio_set(GPIOB, GPIO5);
        step_i(&spiky_i, 10 * spiky_i.scale, 10);
        gpio_clear(GPIOB, GPIO5);
    }
}
