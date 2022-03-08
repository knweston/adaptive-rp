#include "index.h"

//========================================================================================================================//
// CLASS INDEX
//========================================================================================================================//
void Index::profile(vector<Access>& history) {
     
    vector<Record> unsorted_table;
    vector<Record> sorted_table;

    // get distinct addresses, sort them and count access frequency
    for (uint64_t i=0; i < history.size(); ++i) {
        // print profiling progress
        if (PRINT_INTERVAL && (i % PRINT_INTERVAL == 0))
            cout << "  Profiled Accesses = " << i << endl;

        // find in sorted table
        int64_t addr_index = -1;
        addr_index = find_addr(history[i].address, sorted_table);

        if (addr_index >= 0) {  // found
            sorted_table[addr_index].frequency++;
            continue;
        }
        
        else {  // if not found in sorted table, check in unsorted table
            for (uint64_t entry=0; entry < unsorted_table.size(); ++entry) {
                if (history[i].address == unsorted_table[entry].address) {
                    unsorted_table[entry].frequency++;
                    addr_index = entry;
                    break;
                }
            }

            if (addr_index < 0) {
                Record new_record;
                new_record.address = history[i].address;
                unsorted_table.push_back(new_record);
            }
        }

        // merge unsorted table to main profile table 
        if (unsorted_table.size() >= 3000) {
            for (uint64_t j=0; j < unsorted_table.size(); ++j) {
                sorted_table.push_back(unsorted_table[j]);
            }

            // sort the merged profile table
            sort(sorted_table.begin(), sorted_table.end());   
            
            // empty unsorted table
            unsorted_table = {}; 
        }
    }

    // merge any leftover records
    if (unsorted_table.size()) {
        for (uint64_t j=0; j < unsorted_table.size(); ++j) {
            sorted_table.push_back(unsorted_table[j]);
        }
        // sort the merged profile table
        sort(sorted_table.begin(), sorted_table.end());
        unsorted_table = {};
    }

    // sort profiled records based on frequency
    sort(sorted_table.begin(), sorted_table.end(), Record::compare_frequency);

    // export to file
    string filename;
    for (uint32_t i=0; i < TRACE_FILE.size()-13; ++i)
        filename += TRACE_FILE[i];
    filename += "FBI-TRACE.txt";

    ofstream export_file(filename);
    for (uint64_t i=0; i < sorted_table.size(); ++i) {
        export_file << sorted_table[i].address << " " << sorted_table[i].frequency << endl;
    }
    export_file.close();
    cout << "Profile data exported to file: " << filename << endl;
}


int64_t Index::find_addr(uint64_t address, vector<Record>& profile_table) {
    if (!profile_table.empty()) {
        Record new_record;
        new_record.address = address;
        return search(profile_table, 0, profile_table.size()-1, new_record);
    }
    else {
        return -1;
    }
}