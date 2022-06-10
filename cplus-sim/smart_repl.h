#ifndef SMART_REPL_H
#define SMART_REPL_H

#include "replacement.h"

#define MAX_SAMPLES 64
#define SIG_SIZE    16

//========================================================================================================================//
// ZERO POSITIVE LEARNING BYPASS POLICY
//========================================================================================================================//
struct zSample {
    uint64_t addr = 0;
    uint64_t ip   = 0;
};

class ZeroPL : public Replacement {
public:
    ZeroPL(int32_t buffer_size=8192);
    ~ZeroPL();
    
    void initialize(vector<Access>& history, Index& indexing);
    uint32_t get_victim(uint64_t address, uint64_t ip, uint32_t set_index, uint64_t *victim_set);
    void update(uint64_t address, uint64_t ip, uint32_t set_index, uint32_t way_index, bool is_hit);
    void save_state(ofstream &file);
    void load_state(ifstream &file);

    void sendSample();
    bool get_prediction(uint64_t address, uint64_t ip);

private:
    uint32_t lru_position[LLC_SET][LLC_WAY];
    zSample* sample_buffer[MAX_SAMPLES];
    uint32_t sample_ptr;
    bool hit_table[LLC_SET][LLC_WAY];
    uint64_t ip_table[LLC_SET][LLC_WAY];

    // connection config
    int32_t m_port;
    int32_t m_sock;
    int32_t m_bsize;
    string m_ipaddr;
};


#endif // SMART_REPL_H


