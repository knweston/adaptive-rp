#ifndef GLOBAL_H
#define GLOBAL_H

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <chrono>
#include <pthread.h>
#include <queue>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <bitset>

// Cache Configurations
#define LLC_SET 2048
#define LLC_WAY 16
#define LOG2_BLOCK_SIZE 6

// Multi-threading
#define MAX_THREADS 1

// Cache Access Types from ChampSim
#define LOAD      0
#define RFO       1
#define PREFETCH  2
#define WRITEBACK 3
#define NUM_TYPES 4

using namespace std;

//========================================================================================================================//
// HELPER CLASSES
//========================================================================================================================//
class Access {
public:
    Access() {}
    uint64_t address = 0;
    uint32_t type    = 0;
    uint64_t ip      = 0;
};

class Record {
public:
    uint64_t address   = 0;
    uint64_t frequency = 1;
    uint32_t set_id    = 0;

    Record() {}
    bool static compare_frequency(const Record &r1, const Record &r2);

    friend bool operator== (const Record &r1, const Record &r2) { return r1.address == r2.address; }
    friend bool operator!= (const Record &r1, const Record &r2) { return r1.address != r2.address; }
    friend bool operator<  (const Record &r1, const Record &r2) { return r1.address <  r2.address; }
    friend bool operator<= (const Record &r1, const Record &r2) { return r1.address <= r2.address; }
    friend bool operator>  (const Record &r1, const Record &r2) { return r1.address >  r2.address; }
    friend bool operator>= (const Record &r1, const Record &r2) { return r1.address >= r2.address; }
};

struct ThreadData {
    vector<uint64_t> *history;
    uint64_t* rdist_arr;
    uint64_t* victim_set;
    uint64_t start_pt;
    uint32_t* way_list;
    
    ThreadData(vector<uint64_t> *hist, uint64_t ptr, uint64_t* set, uint64_t rd[], uint32_t list[])
        :history(hist), start_pt(ptr), victim_set(set), rdist_arr(rd), way_list(list) {}
};

//========================================================================================================================//
// HELPER FUNCTIONS
//========================================================================================================================//
void init_global_history();
int check_hit(uint64_t address, uint32_t set_index);
void print_final_stats();
int lg2(int n);
int64_t search(vector<Record> &array, int64_t left_pt, int64_t right_pt, Record &element);
void* compute_distance(void* arg);
void create_checkpoint(ofstream& cp_file);
void load_checkpoint(ifstream& cp_file);
void export_eviction_result();

int32_t createSocket();
int32_t readPortConfig(string filename);
string readIPConfig(string filename);
double compute_mean(uint64_t* array, uint64_t array_size);
double compute_std_dev(uint64_t* array, uint64_t array_size);
string flatten(double* array, uint64_t end_pt, uint64_t start_pt=0);
void reset_statistics();
void int_to_bin(uint32_t integer, vector<double>& bin_array);


string sendMessage(int32_t sock, int32_t bsize, string msg);
string getReply(int32_t sock, int32_t bsize);
void   connectServer(int32_t port, int32_t sock, string ipaddr);
void   disconnectServer(int32_t sock, int32_t bsize);

//========================================================================================================================//
// GLOBAL VARIABLES
//========================================================================================================================//
extern vector<Access> GLOBAL_HISTORY;
extern uint64_t CACHE[LLC_SET][LLC_WAY];

extern uint64_t ACCESSES[NUM_TYPES];
extern uint64_t HITS[NUM_TYPES];
extern uint64_t MISSES[NUM_TYPES];
extern uint64_t EVICTIONS[LLC_SET];

extern uint64_t BYPASSES;
extern uint32_t NUM_THREADS;
extern uint32_t PRINT_INTERVAL;
extern uint32_t CHECKPOINT;
extern uint64_t CURRENT_PTR;
extern uint64_t WARMUP_INST;
extern string   TRACE_FILE;
extern string   INDEXING_SCHEME;
extern string   REPL_POLICY;
extern string   CONFIG_FILE;

#endif // GLOBAL_H