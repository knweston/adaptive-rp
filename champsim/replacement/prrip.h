#ifndef PRRIP_H
#define PRRIP_H

#include "champsim.h"

#define SIGNATURE_SIZE 16
#define NUM_PERCEPTRON LLC_SET

class Perceptron {
public:
    Perceptron() {}
    ~Perceptron() {}
    void init_params();
    void compute();
    void train();
    void assign_input(uint32_t* in_array, uint32_t in_size);
    bool get_prediction() {return m_pred;}
    double get_output() {return m_out;}
    
    static double random_wgt() { return rand() / double(RAND_MAX); }
    
private:
    uint32_t m_in[SIGNATURE_SIZE+1] = {0};
    double m_wgts[SIGNATURE_SIZE+1] = {0};
    double m_out = 0;
    bool m_pred  = 0;
};


class PRRIP {
public:
    void init_policy();
    void update(uint32_t addr, bool is_hit);

private:
    Perceptron perceptrons[NUM_PERCEPTRON];
    bool hit_bit[LLC_SET][LLC_WAY];
};

/*
    PRRIP: Perceptron-based RRIP
    - There is N perceptrons, hashed by the setID of the address.
    - Input  = binary of (tag + setID - perceptron list hashed index)
    - Output = 0 (rrpv=maxRRPV) or 1 (rrpv=maxRRPV-1)
    - Each cache line is augmented with a hit bit
    - On a cache hit, switch the hit bit to 1.
    - New cache line has its hit bit set to 0.
    - Training policy: 
        1. upon a hit: make prediction y_pred, if != hit_bit => retrain
        2. upon an eviction: make prediction y_pred, if != hit_bit => retrain
        3. right now the perceptron is trained upon each hit/eviction.
           training policy is the same in both cases.
        4. if the prediction is correct => do not train.
*/


#endif
