#include <stdio.h>

#include "./izhi.h"

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
            step_i(&spiky_i, 10 * spiky_i.scale, 10);
        }
        printf("%f %f %f %f %f\n", i * 0.1, spiky_f.potential,
               (float_t)(spiky_i.potential) / spiky_i.scale,
               spiky_f.recovery, (float_t)(spiky_i.recovery) / spiky_i.scale);
    }
}
