#ifndef FREQUENCY_INDEX_H
#define FREQUENCY_INDEX_H

#include "index.h"

class FrequencyIndex : public Index {
private:
    vector<Record> profile_table;
    uint64_t SET_ACCESSES[LLC_SET] = {0};
public:
    FrequencyIndex() {};
    ~FrequencyIndex() {};
    void initialize(vector<Access>& history);
    uint32_t get_set(uint64_t address);
    void update(uint64_t address) {};
};

#endif // FREQUENCY_INDEX_H