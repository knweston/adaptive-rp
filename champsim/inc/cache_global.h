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

#endif // CACHE_GLOBAL_H