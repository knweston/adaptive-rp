#include "smart_repl.h"

ZeroPL::ZeroPL(int32_t buffer_size) {
    m_port   = readPortConfig(CONFIG_FILE);
    m_ipaddr = readIPConfig(CONFIG_FILE);
    m_bsize  = buffer_size;
    m_sock   = createSocket();

    connectServer(m_port, m_sock, m_ipaddr);
    sample_ptr = 0;

    for (uint32_t i=0; i < MAX_SAMPLES; ++i)
        sample_buffer[i] = new zSample();
}

ZeroPL::~ZeroPL() {
    for (uint32_t i=0; i < MAX_SAMPLES; ++i) {
        delete sample_buffer[i];
    }
    disconnectServer(m_sock, m_bsize);
}

void ZeroPL::initialize(vector<Access>& history, Index& indexing) {
    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way) {
            lru_position[set][way] = way;
            hit_table[set][way]    = 0;
            ip_table[set][way]     = 0;
        }
    }
}

uint32_t ZeroPL::get_victim(uint64_t address, uint64_t ip, uint32_t set_index, uint64_t *victim_set) {
    // fill invalid lines first
    for (uint32_t way=0; way < LLC_WAY; ++way) {
        if (victim_set[way] == 0) // invalid: 0
            return way;
    }

    // do not bypass WRITEBACK instructions (ip==0)
    bool bypass = false;
    if (ip)
        bypass = get_prediction(address, ip);
    
    if (bypass)
        return LLC_WAY;

    else {
        // find replacement candidate
        uint32_t victim = 0;
        for (uint32_t way=0; way < LLC_WAY; ++way) {
            if (lru_position[set_index][way] == (LLC_WAY - 1)) {
                victim = way;
                break;
            }
        }
        
        // create new sample if the block is never re-referenced
        if (!hit_table[set_index][victim]) {
            sample_buffer[sample_ptr]->addr = victim_set[victim];
            sample_buffer[sample_ptr]->ip   = ip_table[set_index][victim];
            ++sample_ptr;
        }

        // send samples to the server when the buffer is full
        if (sample_ptr == MAX_SAMPLES) {
            sendSample();
            sample_ptr = 0;
        }

        return victim;
    }
}

void ZeroPL::update(uint64_t address, uint64_t ip, uint32_t set_index, uint32_t way_index, bool is_hit) {

    // The LRU position is updated regardless of hit/miss
    uint32_t lru_hit_position = lru_position[set_index][way_index];
    for (uint32_t way=0; way < LLC_WAY; ++way) {
        if (lru_position[set_index][way] < lru_hit_position)
            lru_position[set_index][way]++;
    }
    lru_position[set_index][way_index] = 0;

    // if it is a miss, this addr/ip is of the newly-filled block => record the ip
    // if it is a hit, switch the hit bit to 1  
    if (!is_hit) {
        hit_table[set_index][way_index] = 0;
        ip_table[set_index][way_index]  = ip;
    }
    else
        hit_table[set_index][way_index] = 1;

    
}

bool ZeroPL::get_prediction(uint64_t address, uint64_t ip) {
    string data  = "make prediction_" + to_string(ip) + "_" + to_string(address);
    string reply = sendMessage(m_sock, m_bsize, data);

    if (reply == "0")
        return false;
    else
        return true;
}

void ZeroPL::sendSample() {
    string reply = "";
    string data  = "new sample_";

    for (uint32_t i=0; i < MAX_SAMPLES; ++i)
        data += to_string(sample_buffer[i]->ip) + "_" + to_string(sample_buffer[i]->addr) + "_";
    data.pop_back();
    reply = sendMessage(m_sock, m_bsize, data);
}

void ZeroPL::save_state(ofstream &file) {
    for (uint32_t set; set < LLC_SET; ++set) {
        for (uint32_t way; way < LLC_WAY; ++way)
            file << lru_position[set][way] << endl;
    }
}

void ZeroPL::load_state(ifstream &file) {
    string line;
    for (uint32_t set; set < LLC_SET; ++set) {
        for (uint32_t way; way < LLC_WAY; ++way) {
            getline(file, line);
            lru_position[set][way] = stoi(line);
        }
    }
}
