#ifndef NEURAL_INDEX_H
#define NEURAL_INDEX_H

#include "index.h"

#define INTERVAL_SIZE      10000
#define SAMPLE_BUFFER_SIZE 3
#define XOR_MASK_SIZE      12
#define MAX_SHIFTS         14
#define STATE_LENGTH       LLC_SET


struct Sample {
    double state[STATE_LENGTH]      = {0};
    double next_state[STATE_LENGTH] = {0};
    uint32_t action = 0;
    int32_t reward = 0;
};

class NeuralIndex : public Index {
public:
    NeuralIndex(int32_t buffer_size=8192);
    ~NeuralIndex();
    void initialize(vector<Access>& history) {};
    uint32_t get_set(uint64_t address);
    void update(uint64_t address);

    void   move_cache_data();   // move current cache data to new places based on the new mapping
    void   get_prediction();
    void   retrain();
    void   sendSample(Sample* sample);

private:
    // connection config
    int32_t m_port;
    int32_t m_sock;
    int32_t m_bsize;
    string m_ipaddr;

    // current XOR mask config
    uint32_t curr_shifts, next_shifts;

    // Runtime access counters
    uint64_t N_ACCESS[STATE_LENGTH] = {0};
    uint64_t D_ACCESS[STATE_LENGTH] = {0};

    Sample* sample_buffer[SAMPLE_BUFFER_SIZE];
    
    // Stats
    uint64_t num_intervals;
};

#endif // NEURAL_INDEX_H