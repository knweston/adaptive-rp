#ifndef PRRIP_H
#define PRRIP_H

#include "cache.h"

#define NUM_PERCEPTRON 1
#define PRIMARY_SIGNATURE_SIZE 15
#define SECONDARY_SIGNATURE_SIZE 20
#define IS_PRIMARY 1
#define IS_SECONDARY 2

class Perceptron {
public:
    Perceptron() {};
    ~Perceptron();
    void init_params(int32_t _inputs);
    void assign_input(int32_t* in_array, int32_t in_size);
    void compute();
    void train(int32_t* x_train, int32_t x_size, int32_t label);
    
    int32_t  get_prediction() {return m_pred;}
    int32_t  get_output() {return m_out;}
    int32_t* get_weights() {return m_wgts;}
    int32_t sign(double input);
    
private:
    int32_t NUM_INPUTS;
    int32_t *m_in;
    int32_t *m_wgts;
    int32_t m_out  = 0;
    int32_t m_pred = 0;
};


class PRRIP {
public:
    PRRIP() {}
    ~PRRIP();
    void    init_policy(uint32_t cache_sets, uint32_t cache_ways, uint32_t sampled_sets, uint32_t sampled_ways);
    void    update(uint32_t addr, uint32_t set, uint32_t way, int32_t is_hit);
    int32_t predict(uint32_t addr);
    
    void    compute_signature(uint32_t addr, int32_t* signature_array, uint32_t sig_length, int8_t model_type);
    void    dump_weights();

    int32_t is_referenced(uint32_t set, uint32_t way) { return hit_bit[set][way]; }
    void    set_referenced_bit(uint32_t set, uint32_t way, int32_t value) { hit_bit[set][way] = value; }

    // Public statistics
    uint64_t num_primary_train   = 0;
    uint64_t num_secondary_train = 0;
    uint64_t num_update          = 0;

private:
    int32_t NUM_SET;
    int32_t NUM_WAY;

    int32_t SAMPLED_SET;
    int32_t SAMPLED_WAY;

    Perceptron primary_learners[NUM_PERCEPTRON];
    Perceptron secondary_learners[NUM_PERCEPTRON];
    int32_t **hit_bit;
};

/*
    PRRIP: Perceptron-based RRIP
    - There is N primary_learners, hashed by the setID of the address.
    - Input  = binary of (tag + setID - perceptron list hashed index)
    - Output = 0 (rrpv=maxRRPV) or 1 (rrpv=maxRRPV-2)
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
