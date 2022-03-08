#include "frequency_index.h"

//========================================================================================================================//
// FREQUENCY-BASED INDEX
//========================================================================================================================//
void FrequencyIndex::initialize(vector<Access>& history) {
    // import profiled data from file
    string filename;
    for (uint32_t i=0; i < TRACE_FILE.size()-13; ++i)
        filename += TRACE_FILE[i];
    filename += "FBI-TRACE.txt";

    uint64_t sum = 0;

    ifstream file(filename);
    if (file) {
        while(!file.eof()) {
            string line;
            uint64_t address, frequency;
            getline(file, line);
            stringstream ss(line);
            ss >> address;
            ss >> frequency;
            Record new_record;
            new_record.address   = address;
            new_record.frequency = frequency;
            profile_table.push_back(new_record);
            sum += frequency;
        }
    }
    else {
        cout << "Profiling file not found " << filename << endl;
        exit(1);
    }
    
    // if (profile_table.size() % 2) { // odd
    //     uint64_t total_pairs = (profile_table.size()-1)/2;
    //     uint64_t last_index  = profile_table.size()-2;
    //     for (uint64_t i=0; i < total_pairs; ++i) {
    //         profile_table[i].set_id = i % LLC_SET;
    //         profile_table[last_index-i].set_id = i % LLC_SET;
    //     }
    //     profile_table[profile_table.size()-1].set_id = 0;
    // }
    // else {  // even
    //     uint64_t total_pairs = profile_table.size()/2;
    //     uint64_t last_index  = profile_table.size()-1;
    //     for (uint64_t i=0; i < total_pairs; ++i) {
    //         profile_table[i].set_id = i % LLC_SET;
    //         profile_table[last_index-i].set_id = i % LLC_SET;
    //     }
    // }

    uint64_t total_pairs = 0;
    uint64_t last_index  = 0;
    uint64_t mean        = sum / LLC_SET;
    
    cout << "mean = " << mean << endl;
    if (profile_table.size() % 2) { // odd
        total_pairs = (profile_table.size()-1)/2;
        last_index  = profile_table.size()-2;
        profile_table[profile_table.size()-1].set_id = LLC_SET-1;
        SET_ACCESSES[LLC_SET-1] += profile_table[profile_table.size()-1].frequency;
    }

    else {  // even
        total_pairs = profile_table.size()/2;
        last_index  = profile_table.size()-1;
    }

    cout << "total pairs = " << total_pairs << endl;
    cout << "last index  = " << last_index  << endl;

    // round 1:
    uint32_t index = 0;
    while (index < LLC_SET) {
        if (index == profile_table.size())
            break;
        else {
            profile_table[index].set_id = index;
            SET_ACCESSES[index] += profile_table[index].frequency;
            index++;
        }
    }

    // from this point on, only distribute addresses to those sets
    // whose total access < mean
    if (index < profile_table.size()) {
        index = LLC_SET;
        while (index < profile_table.size()) {
            for (uint32_t set=(LLC_SET-1); set > 0; --set) {
                if (index == profile_table.size())
                    break;
                else if (SET_ACCESSES[set] < mean) {
                    if (profile_table[index].frequency == 1) {
                        SET_ACCESSES[LLC_SET-1] += profile_table[index].frequency;
                        profile_table[index].set_id = LLC_SET;
                    }
                    else {
                        SET_ACCESSES[set] += profile_table[index].frequency;
                        profile_table[index].set_id = set;
                    }
                    index++;
                }
            }
        }
    }

    // sort profiled records based on address
    sort(profile_table.begin(), profile_table.end());
}

uint32_t FrequencyIndex::get_set(uint64_t address) {
    int64_t addr_index = find_addr(address, profile_table);

    if (addr_index < 0)
        cout << "Address not found " << address << endl;

    // sanity check
    assert(addr_index >= 0);

    return profile_table[addr_index].set_id;
}
