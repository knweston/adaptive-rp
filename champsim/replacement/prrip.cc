#include "prrip.h"

Perceptron::init_params() {
    for (int i=0; i < SIGNATURE_SIZE+1; ++i)
        m_wgts[i] = random_wgt();
    m_in[0] = 1;
}