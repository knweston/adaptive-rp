#include "lru.h"

//========================================================================================================================//
// LRU POLICY
//========================================================================================================================//
void LRU::initialize(vector<Access>& history, Index& indexing) {
    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way)
            lru_position[set][way] = way;
    }
}

uint32_t LRU::get_victim(uint64_t address, uint64_t ip, uint32_t set_index, uint64_t *victim_set) {
    uint32_t victim = 0;
    for (uint32_t way=0; way < LLC_WAY; ++way) {
        if (lru_position[set_index][way] == (LLC_WAY - 1)) {
            victim = way;
            break;
        }
    }
    return victim;
}

void LRU::update(uint64_t address, uint64_t ip, uint32_t set_index, uint32_t way_index, bool is_hit) {
    uint32_t lru_hit_position = lru_position[set_index][way_index];
    uint32_t total = 0;
    
    for (uint32_t way=0; way < LLC_WAY; ++way) {
        if (lru_position[set_index][way] < lru_hit_position)
            lru_position[set_index][way]++;
    }

    lru_position[set_index][way_index] = 0;
}

void LRU::save_state(ofstream &file) {
    for (uint32_t set; set < LLC_SET; ++set) {
        for (uint32_t way; way < LLC_WAY; ++way)
            file << lru_position[set][way] << endl;
    }
}

void LRU::load_state(ifstream &file) {
    string line;
    for (uint32_t set; set < LLC_SET; ++set) {
        for (uint32_t way; way < LLC_WAY; ++way) {
            getline(file, line);
            lru_position[set][way] = stoi(line);
        }
    }
}