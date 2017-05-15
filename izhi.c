#include "./izhi.h"

void RS_f(fneuron_t *neuron) {
    // create a "regular spiking" floating point neuron
    neuron->a = 0.02;
    neuron->b = 0.2;
    neuron->c = -65;
    neuron->d = 2;
    neuron->potential = neuron->recovery = 0;
}

void step_f(fneuron_t *neuron, float_t synapse, float_t ms) {
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

#define SQRT_FSCALE 1000
#define FSCALE (SQRT_FSCALE * SQRT_FSCALE)

void RS_i(ineuron_t *neuron) {
    neuron->a_inv = 50;
    neuron->b_inv = 5;
    neuron->c = -65 * FSCALE;
    neuron->d = 2 * FSCALE;
    neuron->potential = neuron->recovery = 0;
    neuron->scale = FSCALE;
}

void step_i(ineuron_t *neuron, fixed_t synapse, fixed_t fracms) {
    // step a neuron by 1/fracms milliseconds. synapse input must be scaled
    //  before being passed to this function.
    if (neuron->potential >= 30 * FSCALE) {
        neuron->potential = neuron->c;
        neuron->recovery += neuron->d;
        return;
    }
    fixed_t v = neuron->potential;
    fixed_t u = neuron->recovery;
    fixed_t partial = (v / SQRT_FSCALE) /  5;
    neuron->potential = v + (partial * partial + 5 * v  + 140 * FSCALE
                             - u + synapse) / fracms;
    neuron->recovery = u + ((v / neuron->b_inv - u) / neuron->a_inv) / fracms;
    return;
}
