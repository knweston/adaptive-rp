#ifndef CACHE_GLOBAL_H
#define CACHE_GLOBAL_H

#include "memory_class.h"

// CACHE TYPE
#define IS_ITLB 0
#define IS_DTLB 1
#define IS_STLB 2
#define IS_L1I  3
#define IS_L1D  4
#define IS_L2C  5
#define IS_LLC  6

// L1 INSTRUCTION CACHE
#define L1I_SET 64
#define L1I_WAY 8
#define L1I_RQ_SIZE 64
#define L1I_WQ_SIZE 64 
#define L1I_PQ_SIZE 32
#define L1I_MSHR_SIZE 8
#define L1I_LATENCY 4

// L1 DATA CACHE
#define L1D_SET 64
#define L1D_WAY 8
#define L1D_RQ_SIZE 64
#define L1D_WQ_SIZE 64 
#define L1D_PQ_SIZE 8
#define L1D_MSHR_SIZE 16
#define L1D_LATENCY 4 

// L2 CACHE
#define L2C_SET 512
#define L2C_WAY 8
#define L2C_RQ_SIZE 32
#define L2C_WQ_SIZE 32
#define L2C_PQ_SIZE 16
#define L2C_MSHR_SIZE 32
#define L2C_LATENCY 8      // 4 (L1I or L1D) + 8 = 12 cycles

// LAST LEVEL CACHE
#define LLC_SET NUM_CPUS*4096
#define LLC_WAY 8
#define LLC_RQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_WQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_PQ_SIZE NUM_CPUS*32
#define LLC_MSHR_SIZE NUM_CPUS*64
#define LLC_LATENCY ((LLC_WAY == 1) ? 24 : ((LLC_WAY == 16) ? 32 : 24))     // 4 (L1I or L1D) + 8 (L2C) + 24/26/32 = 36/38/44 cycles

#define BASELINE_RUN 1
#define PRIME ((LLC_SET == 4096) ? 4093 : ((LLC_SET == 8192) ? 8191 : 16381))


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


#endif // CACHE_GLOBAL_H

