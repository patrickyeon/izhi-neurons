#ifndef IZHI_H
#define IZHI_H

#include <stdint.h>

typedef float float_t;
typedef struct {
    float_t a, b, c, d;
    float_t potential, recovery;
} fneuron_t;

void RS_f(fneuron_t *neuron);
void IB_f(fneuron_t *neuron);
void CH_f(fneuron_t *neuron);
void FS_f(fneuron_t *neuron);
void LTS_f(fneuron_t *neuron);
void RZ_f(fneuron_t *neuron);
void TC_f(fneuron_t *neuron);
void step_f(fneuron_t *neuron, float_t synapse, float_t ms);


typedef int32_t fixed_t;
typedef struct {
    // using 1/a, 1/b because a and b are small fractions
    fixed_t a_inv, b_inv, c, d;
    fixed_t potential, recovery;
    fixed_t scale;
} ineuron_t;

void RS_i(ineuron_t *neuron);
void IB_i(ineuron_t *neuron);
void CH_i(ineuron_t *neuron);
void FS_i(ineuron_t *neuron);
void LTS_i(ineuron_t *neuron);
void RZ_i(ineuron_t *neuron);
void TC_i(ineuron_t *neuron);
void step_i(ineuron_t *neuron, fixed_t synapse, uint8_t fracms);

#endif // IZHI_H
