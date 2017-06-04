#include "./izhi.h"

static void base_f(fneuron_t *neuron) {
    // create a "regular spiking" floating point neuron
    neuron->a = 0.02;
    neuron->b = 0.2;
    neuron->c = -65;
    neuron->d = 2;
    neuron->potential = neuron->recovery = 0;
}

void RS_f(fneuron_t *neuron) {
    base_f(neuron);
    neuron->d = 8;
}

void IB_f(fneuron_t *neuron) {
    base_f(neuron);
    neuron->c = -55;
    neuron->d = 4;
}

void CH_f(fneuron_t *neuron) {
    base_f(neuron);
    neuron->c = -50;
}

void FS_f(fneuron_t *neuron) {
    base_f(neuron);
    neuron->a = 0.1;
}

void LTS_f(fneuron_t *neuron) {
    base_f(neuron);
    neuron->b = 0.25;
}

void RZ_f(fneuron_t *neuron) {
    base_f(neuron);
    neuron->a = 0.1;
    neuron->b = 0.26;
}

void TC_f(fneuron_t *neuron) {
    base_f(neuron);
    neuron->b = 0.25;
    neuron->d = 0.05;
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

// XXX Note: if you change LOG_SQRT_FSCALE, you'll need to re-check the math
// XXX        to confirm that it won't overflow, as well as play with those
// XXX        constants when calculating `partial`.
#define LOG_SQRT_FSCALE 10
#define SQRT_FSCALE (1 << LOG_SQRT_FSCALE)
#define FSCALE (SQRT_FSCALE * SQRT_FSCALE)

static void base_i(ineuron_t *neuron) {
    neuron->a_inv = 50;
    neuron->b_inv = 5;
    neuron->c = -65 * FSCALE;
    neuron->d = 2 * FSCALE;
    neuron->potential = neuron->recovery = 0;
    neuron->scale = FSCALE;
}

void RS_i(ineuron_t *neuron) {
    base_i(neuron);
    neuron->d = 8 * FSCALE;
}

void IB_i(ineuron_t *neuron) {
    base_i(neuron);
    neuron->c = -55 * FSCALE;
    neuron->d = 4;
}

void CH_i(ineuron_t *neuron) {
    base_i(neuron);
    neuron->c = -50 * FSCALE;
}

void FS_i(ineuron_t *neuron) {
    base_i(neuron);
    neuron->a_inv = 10;
}

void LTS_i(ineuron_t *neuron) {
    base_i(neuron);
    neuron->b_inv = 4;
}

void RZ_i(ineuron_t *neuron) {
    base_i(neuron);
    neuron->a_inv = 10;
    neuron->b_inv = 4; // should be 1/0.26, we'll see if this is close enough
}

void TC_i(ineuron_t *neuron) {
    base_i(neuron);
    neuron->d = FSCALE / 20;
    neuron->b_inv = 4;
}

void step_i(ineuron_t *neuron, fixed_t synapse, uint8_t fracms) {
    // step a neuron by 2**(-fracms) milliseconds. synapse input must be scaled
    //  before being passed to this function.
    if (neuron->potential >= 30 * FSCALE) {
        neuron->potential = neuron->c;
        neuron->recovery += neuron->d;
        return;
    }
    fixed_t v = neuron->potential;
    fixed_t u = neuron->recovery;
    //fixed_t partial = (v / SQRT_FSCALE) / 5;
    fixed_t partial = ((v >> LOG_SQRT_FSCALE) * 819) >> 12;
    neuron->potential = v + ((partial * partial + 5 * v  + 140 * FSCALE
                             - u + synapse) >> fracms);
    //neuron->recovery = u + (((v / neuron->b_inv - u) / neuron->a_inv)
    //                        >> fracms);
    neuron->recovery = u + (((v - u * neuron->b_inv)
                             / (neuron->b_inv * neuron->a_inv)) >> fracms);
    return;
}
