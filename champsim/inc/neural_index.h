#ifndef NEURAL_INDEX_H
#define NEURAL_INDEX_H

#include "cache_global.h"
  
extern string CONFIG_FILE;

struct Sample {
    double*  state      = NULL;
    double*  next_state = NULL;
    uint32_t action     = 0;
    float    reward     = 0;

    Sample() {}
    Sample(double* _s, double* _ns)
        :state(_s), next_state(_ns) {}

    ~Sample() {
        delete[] state;
        delete[] next_state;
    }
};

struct Pair {
    Pair(uint64_t a, uint64_t b)
        :val_a(a), val_b(b) {}
    uint64_t val_a = 0;
    uint64_t val_b = 0;

    bool static cmp_val_a(const Pair &p1, const Pair &p2) { return p1.val_a > p2.val_a; }
    bool static cmp_val_b(const Pair &p1, const Pair &p2) { return p1.val_b > p2.val_b; }
};

struct Action {
    Action(uint32_t a1, uint32_t a2, uint32_t a3)
        :lo(a1), mi(a2), hi(a3) {}
    uint32_t lo = 0;
    uint32_t mi = 0;
    uint32_t hi = 0;
};


//========================================================================//
// CLASS NEURAL_INDEX
//========================================================================//
class NeuralIndex {
public:
    NeuralIndex(uint8_t type, uint32_t state_length, uint32_t remap_length, BLOCK **arr, uint32_t cpu);
    ~NeuralIndex();
    
    uint32_t get_set(uint64_t address);
    void     update(uint64_t address, MEMORY *dram);
    void     increment_eviction(uint32_t set) { EVICTION[set]++; }
    void     verify_set(uint32_t set);
    void     verify_all_set();

    uint32_t get_map_ptr()    { return map_ptr; }
    uint32_t get_last_shift() { return last_shift; }
    uint32_t get_curr_shift() { return curr_shift; }
    void     move_cache_data(MEMORY *lower_level);   // move data from old set to new set
    uint32_t get_prediction();
    void     retrain();
    void     sendSample(Sample* sample);
    void     print_stats();
    
    void     init_action_table();
    uint32_t compute_index(uint32_t index_bits, uint32_t mask_idx);

    
    static void   init_connection(uint32_t buffer_size=8192);
    static void   close_connection(vector<string> models);
    static string sendMessage(string msg);
    static string getReply();

    // replacement policy
    uint32_t get_lru_victim(uint32_t set);
    void     lru_update(uint32_t set, uint32_t way);

private:
    // connection config
    static int32_t m_port;
    static int32_t m_sock;
    static int32_t m_bsize;
    static string  m_ipaddr;
    
    // current XOR mask config
    uint32_t curr_shift;
    uint32_t next_shift;
    uint32_t last_shift;
    vector<Action> encoded_masks;

    // runtime access counters
    uint64_t* N_ACCESS;
    uint64_t* D_ACCESS;
    uint64_t* EVICTION;

    Sample* sample_buffer;
    
    // runtime stats
    uint64_t access_ctr    = 0;
    uint32_t map_ptr       = 0;
    uint64_t num_interval  = 0;
    uint64_t num_writeback = 0;
    uint64_t num_drop      = 0;
    uint64_t num_read      = 0;

    // preset configurations
    string   MODULE_ID;
    uint32_t FILL_LEVEL;
    uint32_t REMAP_INTERVAL;
    uint32_t INTERVAL_SIZE;
    uint32_t SAMPLE_BUFFER_SIZE;
    uint32_t XOR_MASK_SIZE;
    uint32_t MAX_SHIFTS;
    uint32_t STATE_LENGTH;
    uint32_t CHUNK_SIZE;
    uint32_t NUM_CHUNKS;
    uint32_t NUM_SET;
    uint32_t NUM_WAY;
    uint32_t CPU_ID;
    uint32_t WINDOW_SIZE;

    uint8_t  cache_type;    // can be IS_LLC, IS_L1D, IS_L2C
    BLOCK**  block_array;   // ptr to the block array of the cache
};

//========================================================================//
// HELPER FUNCTIONS
//========================================================================//
int32_t readPortConfig(string filename);
string readIPConfig(string filename);
int32_t createSocket();
double compute_mean(uint64_t* array, uint32_t array_size);
double compute_std_dev(uint64_t* array, uint32_t array_size, double mean);
string flatten(double* array, uint64_t end_pt, uint64_t start_pt);
void override_block(BLOCK *src, BLOCK *des);
vector<uint64_t> get_significant_indices(uint64_t* array, uint32_t array_size, double mean, uint32_t num_select);
uint64_t compute_sum(uint64_t* array, uint32_t array_size);

#endif // NEURAL_INDEX_H