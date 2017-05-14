#include <stdint.h>
#include <stdio.h>

typedef float float_t;

typedef struct {
    float_t a, b, c, d;
    float_t potential, recovery;
} fneuron_t;

static void RS(fneuron_t *neuron) {
    // create a "regular spiking" floating point neuron
    neuron->a = 0.02;
    neuron->b = 0.2;
    neuron->c = -65;
    neuron->d = 2;
    neuron->potential = neuron->recovery = 0;
}

static void step_f(fneuron_t *neuron, float_t synapse, float_t ms) {
    // step a neuron through ms milliseconds with synapse input
    //   if you don't have a good reason to do otherwise, keep ms between 0.1
    //   and 1.0
    if (neuron->potential >= 30) {
        neuron->potential = neuron->c;
        neuron->recovery += neuron->d;
        return;
    }
    float_t v = neuron->potential;
    float_t u = neuron->recovery;
    neuron->potential = v + ms * (0.04 * v * v + 5 * v + 140 - u + synapse);
    neuron->recovery = u + ms * (neuron->a * (neuron->b * v - u));
    return;
}

int main(void) {
    fneuron_t spiky;
    RS(&spiky);
    for (int i = 0; i < 2000; i++) {
        if (i < 100) {
            step_f(&spiky, 0, 0.1);
        } else {
            step_f(&spiky, 10, 0.1);
        }
        printf("%f %f\n", i * 0.1, spiky.potential);
    }
}
