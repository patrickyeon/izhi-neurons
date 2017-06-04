#include <stdio.h>

#include "./izhi.h"

int main(void) {
    fneuron_t spiky_f[7];
    ineuron_t spiky_i[7];

    RS_f(&spiky_f[0]);
    IB_f(&spiky_f[1]);
    CH_f(&spiky_f[2]);
    FS_f(&spiky_f[3]);
    LTS_f(&spiky_f[4]);
    RZ_f(&spiky_f[5]);
    TC_f(&spiky_f[6]);

    RS_i(&spiky_i[0]);
    IB_i(&spiky_i[1]);
    CH_i(&spiky_i[2]);
    FS_i(&spiky_i[3]);
    LTS_i(&spiky_i[4]);
    RZ_i(&spiky_i[5]);
    TC_i(&spiky_i[6]);

    for (int i = 0; i < 5000; i++) {
        if (i < 100) {
            for (int j = 0; j < 7; j++) {
                step_f(&spiky_f[j], 0, 0.125);
                step_i(&spiky_i[j], 0, 3);
            }
        } else {
            for (int j = 0; j < 7; j++) {
                step_f(&spiky_f[j], 10, 0.125);
                step_i(&spiky_i[j], 10 * spiky_i[j].scale, 3);
            }
        }
        printf("%f ", i * 0.1);
        for (int j = 0; j < 7; j++) {
            printf("%f %f ", spiky_f[j].potential,
                   (float_t)(spiky_i[j].potential) / spiky_i[j].scale);
        }
        printf("\n");
    }
}
