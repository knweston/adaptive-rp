#include "belady.h"
#include <iostream>

//========================================================================================================================//
// BELADY POLICY
//========================================================================================================================//
void Belady::initialize(vector<Access>& history, Index& indexing) {
    for (uint64_t access=0; access < history.size(); ++access) {
        // get accessing set
        uint32_t set_index = indexing.get_set(history[access].address);

        // save the address to the history of the corresponding set
        set_history[set_index].push_back(history[access].address);
    }

    for (uint32_t set=0; set < LLC_SET; ++set)
        exec_tracker[set] = 0;
}

uint32_t Belady::get_victim(uint64_t address, uint64_t ip, uint32_t set_index, uint64_t *victim_set) {
    // sanity check
    assert(address == set_history[set_index][exec_tracker[set_index]]);

    // fill invalid lines first
    for (uint32_t way=0; way < LLC_WAY; ++way) {
        if (victim_set[way] == 0 || victim_set[way] == 1) // invalid: 0, dead: 1
            return way;
    }

    // Compare reuse distances of all ways in set (no bypassing)
    uint32_t victim       = 0;
    uint64_t max_distance = 0;
    uint64_t upper_bound  = set_history[set_index].size()-exec_tracker[set_index];

    // Multi-threading
    if (NUM_THREADS > 1) {
        pthread_t   threads[NUM_THREADS];
        uint32_t    way_ids[NUM_THREADS][LLC_WAY/NUM_THREADS];
        uint64_t    reuse_distance[LLC_WAY] = {0};
        vector<ThreadData*> data;

        for (uint32_t tid=0; tid < NUM_THREADS; ++tid) {
            for (uint32_t way=0; way < LLC_WAY/NUM_THREADS; ++way) {
                way_ids[tid][way] = way + tid*(LLC_WAY/NUM_THREADS);
            }
            data.push_back(new ThreadData(&set_history[set_index], exec_tracker[set_index], victim_set, reuse_distance, way_ids[tid]));
        }

        for (uint32_t tid=0; tid < NUM_THREADS; ++tid) {
            while(pthread_create(&(threads[tid]), NULL, &compute_distance, (void*) data[tid]) != 0)
                pthread_join(threads[tid], NULL);
        }

        for (uint32_t tid=0; tid < NUM_THREADS; ++tid)
            pthread_join(threads[tid], NULL);

        for (uint32_t way=0; way < LLC_WAY; ++way) {
            if (reuse_distance[way] > max_distance) {
                victim = way;
                max_distance = reuse_distance[way];
            }
            if (reuse_distance[way] == upper_bound) {
                victim_set[way] = 1;    // mark as dead, no need to compute distance next time
            }
        }
        for (uint32_t tid=0; tid < NUM_THREADS; ++tid)
            delete data[tid];
    }

    // Single-threading
    else {
        for (uint32_t way=0; way < LLC_WAY; ++way) {
            uint64_t entry=exec_tracker[set_index];
            for (entry; entry < set_history[set_index].size(); ++entry) {
                if (victim_set[way] == set_history[set_index][entry]) {
                    uint64_t distance = entry - exec_tracker[set_index];
                    if (distance > max_distance) {
                        victim = way;
                        max_distance = distance;
                    }
                    break;
                }
            }
            if (entry == set_history[set_index].size()) {
                victim_set[way] = 1;    // mark as dead, no need to compute distance next time
            }
        }

        for (uint32_t way=0; way < LLC_WAY; ++way) {
            if (victim_set[way] == 1) {
                victim = way;
                break;
            }
        }

        return victim;
    }

    
    return victim;
}

void Belady::update(uint64_t address, uint64_t ip, uint32_t set_index, uint32_t way_index, bool is_hit) {
    exec_tracker[set_index]++;
}

void Belady::save_state(ofstream &file) {
    for (uint32_t set=0; set < LLC_SET; ++set)
        file << exec_tracker[set] << endl;
}

void Belady::load_state(ifstream &file) {
    string line;
    for (uint32_t set=0; set < LLC_SET; ++set) {
        getline(file, line);
        exec_tracker[set] = stoi(line);
    }
}
