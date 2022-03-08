#include "belady.h"
#include "lru.h"
#include "frequency_index.h"
#include "normal_index.h"
#include "neural_index.h"
#include "smart_repl.h"

//========================================================================================================================//
// GLOBAL VARIABLES
//========================================================================================================================//
vector<Access> GLOBAL_HISTORY;
uint64_t CACHE[LLC_SET][LLC_WAY];

uint64_t ACCESSES[NUM_TYPES];
uint64_t HITS[NUM_TYPES];
uint64_t MISSES[NUM_TYPES];
uint64_t EVICTIONS[LLC_SET];

uint64_t BYPASSES          = 0;
uint32_t NUM_THREADS     = LLC_WAY > MAX_THREADS ? MAX_THREADS : LLC_WAY;
uint32_t PRINT_INTERVAL  = 0;
uint64_t CURRENT_PTR     = 0;
uint32_t CHECKPOINT      = 0;
string   TRACE_FILE      = "";
string   INDEXING_SCHEME = "NORMAL";
string   REPL_POLICY     = "LRU";
string   CONFIG_FILE     = "";

//========================================================================================================================//
// MAIN FUNCTION
//========================================================================================================================//
int main(int argc, char *argv[]) {
    if (argc < 6) {
        cout << "Usage: " << argv[0] << " [REPL_POLICY] [INDEXING_SCHEME] [TRACE_FILE]  [CHECKPOINT] [PRINT_INTERVAL] [CONFIG_FILE]" << endl;
        exit(1);
    }

    // set simulation parameters
    REPL_POLICY     = argv[1];
    INDEXING_SCHEME = argv[2];
    TRACE_FILE      = argv[3];
    CHECKPOINT      = stoi(argv[4]);
    PRINT_INTERVAL  = stoi(argv[5]);

    if (argc > 6) {
        CONFIG_FILE = argv[6];
    }

    // sanity check
    if (INDEXING_SCHEME == "NEURAL" && argc < 7) {
        cout << "Index " << INDEXING_SCHEME << "requires a socket configuration file" << endl;
        cout << "Usage: " << argv[0] << "[REPL_POLICY] [INDEXING_SCHEME] [TRACE_FILE]  [CHECKPOINT] [PRINT_INTERVAL] [CONFIG_FILE]" << endl;
        exit(1);
    }
    
    // initialize LLC cache
    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way)
            CACHE[set][way] = 0;
    }

    // initialize global history using provided trace
    init_global_history();   
    cout << "Global History initialization complete" << endl;

    // initialize indexing (frequency-based indexing only)
    Index *indexing;
    if (INDEXING_SCHEME == "FBI")
        indexing = new FrequencyIndex();
    else if (INDEXING_SCHEME == "NORMAL")
        indexing = new NormalIndex();
    else if (INDEXING_SCHEME == "NEURAL")
        indexing = new NeuralIndex();
    else if (INDEXING_SCHEME == "PROFILE") {
        indexing->profile(GLOBAL_HISTORY);
        cout << "Index Profiling complete" << endl;
        return 0;   // Profile addresses then finish simulation
    }
    else {
        cout << "Unrecognized indexing scheme: " << INDEXING_SCHEME << endl;
        exit(1);
    }
    indexing->initialize(GLOBAL_HISTORY);
    cout << "Frequency-Based Indexing initialization complete" << endl;

    // initialize replacement policy
    Replacement *replacement_policy;
    
    if (REPL_POLICY == "BELADY")
        replacement_policy = new Belady();
    else if (REPL_POLICY == "LRU")
        replacement_policy = new LRU();
    else if (REPL_POLICY == "ZPL")
        replacement_policy = new ZeroPL();
    else {
        cout << "Unrecognized replacement policy: " << REPL_POLICY << endl;
        exit(1);
    }
    replacement_policy->initialize(GLOBAL_HISTORY, *indexing);
    cout << "Replacement policy initialization complete" << endl;

    // load previous checkpoint (if CHECKPOINT is ON)
    if (CHECKPOINT) {
        // open checkpoint file
        string filename;
        for (uint32_t i=0; i < TRACE_FILE.size()-13; ++i)
            filename += TRACE_FILE[i];
        filename += "CHECKPOINT.txt";
        ifstream cp_file(filename);

        if (cp_file) {
            load_checkpoint(cp_file);                 // load simulation data and settings 
            replacement_policy->load_state(cp_file);  // load replacement policy data
            cout << "Checkpoint loaded successfully" << endl;
        }
        cp_file.close();
    }

    auto start = chrono::steady_clock::now();

    // main LLC simulation
    for (CURRENT_PTR; CURRENT_PTR < GLOBAL_HISTORY.size(); ++CURRENT_PTR) {
        
        // Save a checkpoint (if CHECKPOINT is ON)
        if (CHECKPOINT && (CURRENT_PTR % CHECKPOINT == 0)) {   
            string filename;
            for (uint32_t i=0; i < TRACE_FILE.size()-13; ++i)
                filename += TRACE_FILE[i];
            filename += "CHECKPOINT.txt";
            ofstream cp_file(filename);
            create_checkpoint(cp_file);               // save simulation data and settings 
            replacement_policy->save_state(cp_file);  // save replacement policy data
            cout << "  Checkpoint saved" << endl;
            cp_file.close();
        } 

        // get current address
        uint64_t current_addr = GLOBAL_HISTORY[CURRENT_PTR].address;

        // print simulation progress
        if (PRINT_INTERVAL && (CURRENT_PTR % PRINT_INTERVAL == 0)) {
            auto end = chrono::steady_clock::now();
            cout << left << "  Processed Accesses = " << setw(12) << CURRENT_PTR;
            cout << "  Interval time (s) = " 
                 << chrono::duration_cast<chrono::seconds>(end - start).count()
                 << " s" << " BYPASSES: " << BYPASSES << endl;
            start = end;
        }

        // get accessing set
        uint32_t set_index = indexing->get_set(current_addr);
        if (set_index >= LLC_SET) {
            cout << "Invalid set index: " << set_index << endl;
            assert(0);
        }


        // check whether this is a hit or miss
        int hit_way = check_hit(current_addr, set_index);
        
        if (hit_way >= 0) { // LLC hit           
            // collect stats
            HITS[GLOBAL_HISTORY[CURRENT_PTR].type]++;
            ACCESSES[GLOBAL_HISTORY[CURRENT_PTR].type]++;
            
            // update replacement policy
            replacement_policy->update(current_addr, GLOBAL_HISTORY[CURRENT_PTR].ip, set_index, hit_way, 1);
        }

        else { // LLC miss
            uint32_t victim_way = replacement_policy->get_victim(current_addr, GLOBAL_HISTORY[CURRENT_PTR].ip, set_index, CACHE[set_index]);

            if (victim_way == LLC_WAY) { // bypass
                // collect stats
                MISSES[GLOBAL_HISTORY[CURRENT_PTR].type]++;
                ACCESSES[GLOBAL_HISTORY[CURRENT_PTR].type]++;
                BYPASSES++;
            }

            else {                      // fill cache
                // collect stats
                MISSES[GLOBAL_HISTORY[CURRENT_PTR].type]++;
                ACCESSES[GLOBAL_HISTORY[CURRENT_PTR].type]++;
                if (CACHE[set_index][victim_way])
                    EVICTIONS[set_index]++;
                // fill cache with new data
                CACHE[set_index][victim_way] = current_addr;
                
                // update replacement policy
                replacement_policy->update(current_addr, GLOBAL_HISTORY[CURRENT_PTR].ip, set_index, victim_way, 0);
            }
        }

        indexing->update(current_addr);

    } // end main simulation
    

    // print LLC final statistics
    print_final_stats();
    delete replacement_policy;
    delete indexing;
    
    return 0;
}
