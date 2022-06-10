#include "cache.h"
#include "prrip.h"

// [Kevin]
// #define maxRRPV 3
// uint32_t rrpv[NUM_CPUS][L1I_SET][L1I_WAY];
// PRRIP pct_module[NUM_CPUS];

// [Kevin]
void CACHE::initialize_replacement() {
    // if (cache_type == IS_L1I) {
    //     for (int n=0; n<NUM_CPUS; n++) {
    //         for (int i=0; i<L1I_SET; i++) {
    //             for (int j=0; j<L1I_WAY; j++) {
    //                 rrpv[n][i][j] = maxRRPV;
    //             }
    //         }
    //         pct_module[n].init_policy(L1I_SET, L1I_WAY);
    //     }
    //     cout << "Initialized L1I PRRIP" << endl;
    // }
}

uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // [Kevin]
    // if (cache_type == IS_L1I) {
    //     // look for the maxRRPV line
    //     while (1)
    //     {            
    //         for (int i=0; i<L1I_WAY; i++)
    //             if (rrpv[cpu][set][i] == maxRRPV)
    //                 return i;

    //         for (int i=0; i<L1I_WAY; i++)
    //             rrpv[cpu][set][i]++;
    //     }
    // }  
    
    // baseline LRU replacement policy for other caches 
    return lru_victim(cpu, instr_id, set, current_set, ip, full_addr, type); 
}

void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    // [Kevin]
    // if (cache_type == IS_L1I) {
    //     // handle writeback access
    //     if (type == WRITEBACK) {
    //         if (hit) {
    //             pct_module[cpu].update(full_addr>>LOG2_BLOCK_SIZE, set, way, 1);   // update upon writeback hit
    //             return;
    //         }
    //         else {
    //             rrpv[cpu][set][way] = maxRRPV-1;
    //             pct_module[cpu].update(victim_addr>>LOG2_BLOCK_SIZE, set, way, -1);   // update upon writeback miss (eviction)
    //             return;
    //         }
    //     }

    //     if (hit) {
    //         rrpv[cpu][set][way] = 0;
    //         pct_module[cpu].update(full_addr>>LOG2_BLOCK_SIZE, set, way, 1);   // update upon hit
    //     }
    //     else {      
    //         if (block[set][way].valid) {  // update upon eviction
    //             pct_module[cpu].update(victim_addr>>LOG2_BLOCK_SIZE, set, way, -1);
    //             rrpv[cpu][set][way] = pct_module[cpu].predict(full_addr>>LOG2_BLOCK_SIZE);
    //         }
    //         else
    //             rrpv[cpu][set][way] = maxRRPV-1;
    //     }
    //     return;
    // }

    if (type == WRITEBACK) {
        if (hit) // wrietback hit does not update LRU state
            return;
    }

    return lru_update(set, way);
}

uint32_t CACHE::lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    uint32_t way = 0;

    // fill invalid line first
    for (way=0; way<NUM_WAY; way++) {
        if (block[set][way].valid == false) {

            DP ( if (warmup_complete[cpu]) {
            cout << "[" << NAME << "] " << __func__ << " instr_id: " << instr_id << " invalid set: " << set << " way: " << way;
            cout << hex << " address: " << (full_addr>>LOG2_BLOCK_SIZE) << " victim address: " << block[set][way].address << " data: " << block[set][way].data;
            cout << dec << " lru: " << block[set][way].lru << endl; });

            break;
        }
    }

    // LRU victim
    if (way == NUM_WAY) {
        for (way=0; way<NUM_WAY; way++) {
            if (block[set][way].lru == NUM_WAY-1) {

                DP ( if (warmup_complete[cpu]) {
                cout << "[" << NAME << "] " << __func__ << " instr_id: " << instr_id << " replace set: " << set << " way: " << way;
                cout << hex << " address: " << (full_addr>>LOG2_BLOCK_SIZE) << " victim address: " << block[set][way].address << " data: " << block[set][way].data;
                cout << dec << " lru: " << block[set][way].lru << endl; });

                break;
            }
        }
    }

    if (way == NUM_WAY) {
        cerr << "[" << NAME << "] " << __func__ << " no victim! set: " << set << endl;
        assert(0);
    }

    return way;
}

void CACHE::lru_update(uint32_t set, uint32_t way)
{
    // update lru replacement state
    for (uint32_t i=0; i<NUM_WAY; i++) {
        if (block[set][i].lru < block[set][way].lru) {
            block[set][i].lru++;
        }
    }
    block[set][way].lru = 0; // promote to the MRU position
}

void CACHE::replacement_final_stats()
{   
    // [Kevin]
    // cout << "==============================" << endl;
    // cout << "Perceptron RRIP Stats:" << endl;
    // cout << "   Num Updates:          " << pct_module[0].num_update << endl;
    // cout << "   Num Primary Trains:   " << pct_module[0].num_primary_train << endl;
    // cout << "   Num Secondary Trains: " << pct_module[0].num_secondary_train << endl;
    // cout << "==============================" << endl;
    // pct_module[0].dump_weights();
}

#ifdef NO_CRC2_COMPILE
void InitReplacementState()
{
    
}

uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    return 0;
}

void UpdateReplacementState (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    
}

void PrintStats_Heartbeat()
{
    
}

void PrintStats()
{

}
#endif
