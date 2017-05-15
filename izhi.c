#include <stdint.h>
#include <stdio.h>

typedef float float_t;

typedef struct {
    float_t a, b, c, d;
    float_t potential, recovery;
} fneuron_t;

static void RS_f(fneuron_t *neuron) {
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

typedef int16_t fixed_t;
#define FSCALE 289
#define SQRT_FSCALE 17

typedef struct {
    // using 1/a, 1/b because a and b are small fractions
    fixed_t a_inv, b_inv, c, d;
    fixed_t potential, recovery;
} ineuron_t;

static void RS_i(ineuron_t *neuron) {
    neuron->a_inv = 50;
    neuron->b_inv = 5;
    neuron->c = -65 * FSCALE;
    neuron->d = 2 * FSCALE;
    neuron->potential = neuron->recovery = 0;
}

static void step_i(ineuron_t *neuron, fixed_t synapse, fixed_t fracms) {
    // step a neuron by 1/fracms milliseconds. synapse input must be scaled
    //  before being passed to this function.
    if (neuron->potential >= 30 * FSCALE) {
        neuron->potential = neuron->c;
        neuron->recovery += neuron->d;
        return;
    }
    fixed_t v = neuron->potential;
    fixed_t u = neuron->recovery;
    fixed_t subpartial = (v / SQRT_FSCALE) /  5;
    fixed_t partial = ((subpartial / 5) * (subpartial / 2) + v / 2
                       + 14 * FSCALE) + (synapse - u) / 10;
    neuron->potential = v + partial;
    neuron->recovery = u + ((v / neuron->b_inv - u) / neuron->a_inv) / fracms;
    return;
}

int main(void) {
    fneuron_t spiky_f;
    ineuron_t spiky_i;
    RS_f(&spiky_f);
    RS_i(&spiky_i);
    for (int i = 0; i < 5000; i++) {
        if (i < 100) {
            step_f(&spiky_f, 0, 0.1);
            step_i(&spiky_i, 0, 10);
        } else {
            step_f(&spiky_f, 10, 0.1);
            step_i(&spiky_i, 10 * FSCALE, 10);
        }
        printf("%f %f %f %f %f\n", i * 0.1, spiky_f.potential,
               (float_t)(spiky_i.potential) / FSCALE,
               spiky_f.recovery, (float_t)(spiky_i.recovery) / FSCALE);
    }
}
