#ifndef REPLACEMENT_H
#define REPLACEMENT_H

#include "global.h"
#include "index.h"

//========================================================================================================================//
// PARENT REPLACEMENT POLICY
//========================================================================================================================//
class Replacement {
public:
    Replacement() {}
    virtual ~Replacement() {};
    virtual void initialize(vector<Access>& history, Index& indexing) = 0;
    virtual uint32_t get_victim(uint64_t address, uint64_t ip, uint32_t set_index, uint64_t *victim_set) = 0;
    virtual void update(uint64_t address, uint64_t ip, uint32_t set_index, uint32_t way_index, bool is_hit) = 0;
    virtual void save_state(ofstream &file) = 0;
    virtual void load_state(ifstream &file) = 0;
};

#endif // REPLACEMENT_H