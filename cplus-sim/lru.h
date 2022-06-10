#ifndef LRU_H
#define LRU_H

#include "replacement.h"

//========================================================================================================================//
// LRU POLICY
//========================================================================================================================//
class LRU : public Replacement {
private:
    uint32_t lru_position[LLC_SET][LLC_WAY];
public:
    LRU() {}
    ~LRU() {}
    void initialize(vector<Access>& history, Index& indexing);
    uint32_t get_victim(uint64_t address, uint64_t ip, uint32_t set_index, uint64_t *victim_set);
    void update(uint64_t address, uint64_t ip, uint32_t set_index, uint32_t way_index, bool is_hit);
    void save_state(ofstream &file);
    void load_state(ifstream &file);
};

#endif // LRU_H