#ifndef INDEX_H
#define INDEX_H

#include "global.h"

//========================================================================================================================//
// FREQUENCY-BASED INDEX
//========================================================================================================================//
class Index {
public:
    Index() {}
    virtual ~Index() {}
    void profile(vector<Access>& history);
    int64_t find_addr(uint64_t address, vector<Record>& profile_table);
    virtual uint32_t get_set(uint64_t address) = 0;
    virtual void initialize(vector<Access>& history) = 0;
    virtual void update(uint64_t address) = 0;
};

#endif // INDEX_H