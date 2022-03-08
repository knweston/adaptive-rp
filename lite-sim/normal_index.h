#ifndef NORMAL_INDEX_H
#define NORMAL_INDEX_H

#include "index.h"

class NormalIndex : public Index {
public:
    NormalIndex() {};
    ~NormalIndex() {};
    void initialize(vector<Access>& history) {}
    uint32_t get_set(uint64_t address) { return address & ((1 << lg2(LLC_SET)) - 1); }
    void update(uint64_t address) {};
};

#endif // NORMAL_INDEX_H