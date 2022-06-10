#include "prrip.h"

//============================================================//
// CLASS PERCEPTRON
//============================================================//
Perceptron::~Perceptron() {
    delete[] m_in;
    delete[] m_wgts;
}

void Perceptron::init_params(int32_t _inputs) {
    NUM_INPUTS = _inputs;
    m_in   = new int32_t[NUM_INPUTS];
    m_wgts = new int32_t[NUM_INPUTS];

    for (int32_t i=0; i < NUM_INPUTS; ++i) {
        m_in[i]   = 0;
        m_wgts[i] = 0;
    }

    m_in[NUM_INPUTS-1] = 1; // the last weight/input is the bias
}

void Perceptron::assign_input(int32_t* in_array, int32_t in_size) {
    if (in_size != (NUM_INPUTS-1))  // sanity check
        assert(0);

    for (int32_t i=0; i < in_size; ++i)
        m_in[i] = in_array[i];
}

void Perceptron::compute() {
    m_out = 0;
    for (int32_t i=0; i < NUM_INPUTS; ++i)
        m_out += m_in[i] * m_wgts[i];

    m_pred = sign(m_out);
}

void Perceptron::train(int32_t* x_train, int32_t x_size, int32_t label) {
    if (x_size != (NUM_INPUTS-1))  // sanity check
        assert(0);

    for (int32_t i=0; i < x_size; ++i)
        m_wgts[i] += label * x_train[i];
    
    m_wgts[x_size] += label;    // bias weight
}

int Perceptron::sign(double input) {
    if (input < 0)
        return -1;
    else
        return 1;
}


//============================================================//
// CLASS PRRIP
//============================================================//
PRRIP::~PRRIP() {
    for (int32_t i=0; i < SAMPLED_SET; ++i)
        delete[] hit_bit[i];
    delete[] hit_bit;
}

void PRRIP::init_policy(uint32_t cache_sets, uint32_t cache_ways, uint32_t sampled_sets, uint32_t sampled_ways) {
    // Initialize cache parameters
    NUM_SET = cache_sets;
    NUM_WAY = cache_ways;
    SAMPLED_SET = sampled_sets;
    SAMPLED_WAY = sampled_ways;

    // Initialize hit table
    hit_bit = new int32_t*[SAMPLED_SET];
    for (int32_t set=0; set < SAMPLED_SET; ++set)
        hit_bit[set] = new int32_t[SAMPLED_WAY];
    
    for (int32_t set=0; set < SAMPLED_SET; ++set) {
        for (int32_t way=0; way < SAMPLED_WAY; ++way)
            hit_bit[set][way] = 0;
    }

    // Initialize all primary_learners
    for (uint32_t i=0; i < NUM_PERCEPTRON; ++i)
        primary_learners[i].init_params(PRIMARY_SIGNATURE_SIZE+1);

    // Initialize all secondary_learners
    for (uint32_t i=0; i < NUM_PERCEPTRON; ++i)
        secondary_learners[i].init_params(SECONDARY_SIGNATURE_SIZE+1);
}

int32_t PRRIP::predict(uint32_t addr) {
    // compute signature array from input address
    int32_t primary_signature[PRIMARY_SIGNATURE_SIZE] = {0};
    int32_t secondary_signature[SECONDARY_SIGNATURE_SIZE] = {0};

    compute_signature(addr, primary_signature, PRIMARY_SIGNATURE_SIZE, IS_PRIMARY);
    compute_signature(addr, secondary_signature, SECONDARY_SIGNATURE_SIZE, IS_SECONDARY);

    // find perceptron (currently perceptron ID == setID)
    uint32_t pid = (uint32_t)(addr & ((1 << lg2(NUM_SET)) - 1)) % NUM_PERCEPTRON;

    // assign inputs to the perceptron
    primary_learners[pid].assign_input(primary_signature, PRIMARY_SIGNATURE_SIZE);
    secondary_learners[pid].assign_input(secondary_signature, SECONDARY_SIGNATURE_SIZE);
    
    // compute prediction
    primary_learners[pid].compute();
    secondary_learners[pid].compute();

    int32_t primary_pred = primary_learners[pid].get_prediction();
    int32_t secondary_pred = secondary_learners[pid].get_prediction();

    if ((primary_pred > 0) && (secondary_pred > 0))         // both models predict cache-friendly
        return 0;
    else if ((primary_pred < 0) && (secondary_pred < 0))    // both models predict cache-averse
        return 3;
    else {                                                  // disagree
        // if 2nd perceptron is better
        if (num_primary_train > num_secondary_train) {
            if (secondary_pred > 0)
                return 1;
            else
                return 2;
        }

        // 1st perceptron is better 
        else
            if (primary_pred > 0)
                return 1;
            else
                return 2;
    }
}

void PRRIP::update(uint32_t addr, uint32_t set, uint32_t way, int32_t is_hit) {
    if (!hit_bit[set][way]) {        
        // compute signature array from input address
        int32_t primary_signature[PRIMARY_SIGNATURE_SIZE] = {0};
        int32_t secondary_signature[SECONDARY_SIGNATURE_SIZE] = {0};

        compute_signature(addr, primary_signature, PRIMARY_SIGNATURE_SIZE, IS_PRIMARY);
        compute_signature(addr, secondary_signature, SECONDARY_SIGNATURE_SIZE, IS_SECONDARY);

        // find perceptron from setID
        uint32_t pid = (uint32_t)(addr & ((1 << lg2(NUM_SET)) - 1)) % NUM_PERCEPTRON;

        // assign inputs to the perceptron
        primary_learners[pid].assign_input(primary_signature, PRIMARY_SIGNATURE_SIZE);
        secondary_learners[pid].assign_input(secondary_signature, SECONDARY_SIGNATURE_SIZE);
        
        // compute prediction
        primary_learners[pid].compute();
        secondary_learners[pid].compute();

        // retrain if mispredicted
        if (primary_learners[pid].get_prediction() != is_hit) {
            primary_learners[pid].train(primary_signature, PRIMARY_SIGNATURE_SIZE, is_hit);
            num_primary_train++;
        }
        if (secondary_learners[pid].get_prediction() != is_hit) {
            secondary_learners[pid].train(secondary_signature, SECONDARY_SIGNATURE_SIZE, is_hit);
            num_secondary_train++;
        }
    }
    if (is_hit > 0)
        hit_bit[set][way] = 1;
    else
        hit_bit[set][way] = 0;
    num_update++;
}


void PRRIP::compute_signature(uint32_t addr, int32_t* signature_array, uint32_t sig_length, int8_t model_type) {
    uint32_t tag, signature;

    if (model_type == IS_PRIMARY) {
        tag = addr >> lg2(NUM_SET);
        signature = tag & ((1 << sig_length) - 1);

        for (uint32_t i=0; i < sig_length; ++i) {
            if ((signature >> i) & 1)
                signature_array[sig_length-1-i] = 1;
            else
                signature_array[sig_length-1-i] = -1;
        }
    }
    else {
        signature = (addr >> 6) & ((1 << sig_length) - 1);  // physical memory frameID (sig_length = 20)
        for (uint32_t i=0; i < sig_length; ++i) {
            if ((signature >> i) & 1)
                signature_array[sig_length-1-i] = 1;
            else
                signature_array[sig_length-1-i] = -1;
        }
    }
}

void PRRIP::dump_weights() {
    for (uint32_t n=0; n < NUM_PERCEPTRON; ++n) {
        cout << "primary_perceptron #" << n << ": ";
        for (int32_t i=0; i < PRIMARY_SIGNATURE_SIZE+1; ++i)
            cout << primary_learners[n].get_weights()[i] << " ";
        cout << endl;
        cout << "secondary_perceptron #" << n << ": ";
        for (int32_t i=0; i < SECONDARY_SIGNATURE_SIZE+1; ++i)
            cout << secondary_learners[n].get_weights()[i] << " ";
        cout << endl << endl;
    }
}