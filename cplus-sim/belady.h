#ifndef BELADY_H
#define BELADY_H

#include "replacement.h"

//========================================================================================================================//
// BELADY POLICY
//========================================================================================================================//
class Belady : public Replacement {
private:
    vector<uint64_t> set_history[LLC_SET];
    uint64_t exec_tracker[LLC_SET];
public:
    Belady() {}
    ~Belady() {}
    void initialize(vector<Access>& history, Index& indexing);
    uint32_t get_victim(uint64_t address, uint64_t ip, uint32_t set_index, uint64_t *victim_set);
    void update(uint64_t address, uint64_t ip, uint32_t set_index, uint32_t way_index, bool is_hit);
    void save_state(ofstream &file);
    void load_state(ifstream &file);
};

#endif // BELADY_H