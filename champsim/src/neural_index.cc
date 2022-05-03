#include "neural_index.h"

//========================================================================//
// STATIC PARAMETERS
//========================================================================//
int32_t NeuralIndex::m_port;
int32_t NeuralIndex::m_sock;
int32_t NeuralIndex::m_bsize;
string  NeuralIndex::m_ipaddr;

//========================================================================//
// CLASS NEURAL_INDEX
//========================================================================//   
NeuralIndex::NeuralIndex(uint8_t type, uint32_t state_length, uint32_t remap_length, BLOCK **arr, uint32_t cpu) {
    cache_type     = type;
    STATE_LENGTH   = state_length;
    REMAP_INTERVAL = remap_length;
    CPU_ID         = cpu;
    block_array    = arr;
    SAMPLE_BUFFER_SIZE = 3;
    last_shift     = 0;
    curr_shift     = 0;
    next_shift     = 0;

    if (cache_type == IS_L1D) {
        NUM_SET       = L1D_SET;
        NUM_WAY       = L1D_WAY;
        XOR_MASK_SIZE = lg2(L1D_SET);
        CHUNK_SIZE    = (state_length > 1024? 1024 : state_length);
        NUM_CHUNKS    = (state_length > CHUNK_SIZE? state_length/CHUNK_SIZE : 1);
        INTERVAL_SIZE = REMAP_INTERVAL * 1000;
        MODULE_ID     = "L1D_"+to_string(CPU_ID);
        FILL_LEVEL    = FILL_L1;
    }
    else if (cache_type == IS_L2C) {
        NUM_SET       = L2C_SET;
        NUM_WAY       = L2C_WAY;
        XOR_MASK_SIZE = lg2(L2C_SET);
        CHUNK_SIZE    = (state_length > 1024? 1024 : state_length);
        NUM_CHUNKS    = (state_length > CHUNK_SIZE? state_length/CHUNK_SIZE : 1);
        INTERVAL_SIZE = REMAP_INTERVAL * 10000;
        MODULE_ID     = "L2C_"+to_string(CPU_ID);
        FILL_LEVEL    = FILL_L2;
    }
    else if (cache_type == IS_LLC) {
        NUM_SET       = LLC_SET;
        NUM_WAY       = LLC_WAY;
        XOR_MASK_SIZE = lg2(LLC_SET);
        CHUNK_SIZE    = (state_length > 1024? 1024 : state_length);
        NUM_CHUNKS    = (state_length > CHUNK_SIZE? state_length/CHUNK_SIZE : 1);
        INTERVAL_SIZE = REMAP_INTERVAL * 35000 * NUM_CPUS;
        MODULE_ID     = "LLC_"+to_string(CPU_ID);
        FILL_LEVEL    = FILL_LLC;
        WINDOW_SIZE   = 5;
    }
    else {
        cout << "Unrecognized cache type: " << type << endl;
        assert(false);
    }
    
    MAX_SHIFTS    = 5*4*3 - 1;
    N_ACCESS      = new uint64_t [NUM_SET];
    D_ACCESS      = new uint64_t [NUM_SET];
    EVICTION      = new uint64_t [NUM_SET];
    sample_buffer = new Sample   [SAMPLE_BUFFER_SIZE];
    for (uint32_t i=0; i < SAMPLE_BUFFER_SIZE; ++i) {
        sample_buffer[i].state = new double[STATE_LENGTH];
        sample_buffer[i].next_state = new double[STATE_LENGTH];
    }

    memset(N_ACCESS, 0, NUM_SET * sizeof(*N_ACCESS));
    memset(D_ACCESS, 0, NUM_SET * sizeof(*D_ACCESS));
    memset(EVICTION, 0, NUM_SET * sizeof(*EVICTION));

    cout << "Preparing mask index" << endl;
    init_action_table();
    cout << "Done mask index" << endl;

    cout << endl;
    cout << "========================================================================" << endl;
    cout << "NEURAL INDEX CONFIGURATION" << endl;
    cout << "========================================================================" << endl;
    cout << "MODULE_ID:        " << MODULE_ID << endl;
    cout << "Config file path: " << CONFIG_FILE << endl;
    cout << "State length:     " << STATE_LENGTH << endl;
    cout << "Chunk size:       " << CHUNK_SIZE << endl;
    cout << "Num chunks:       " << NUM_CHUNKS << endl;
    cout << "Remap interval:   " << REMAP_INTERVAL << endl;
    cout << "Num sets:         " << NUM_SET << endl;
    cout << "Num ways:         " << NUM_WAY << endl;
    cout << "XOR mask size:    " << XOR_MASK_SIZE << endl;
    cout << "Max shifts:       " << MAX_SHIFTS << endl;
    cout << "========================================================================" << endl;
    cout << endl;
}


//========================================================================//
NeuralIndex::~NeuralIndex() {
    delete[] sample_buffer;
    delete[] N_ACCESS;
    delete[] D_ACCESS;
    delete[] EVICTION;
}

//========================================================================//
void NeuralIndex::update(uint64_t address, MEMORY *lower_level) {
    uint32_t neural_set = get_set(address);
    uint32_t normal_set = address & ((1 << lg2(NUM_SET)) - 1);
    N_ACCESS[normal_set]++;
    D_ACCESS[neural_set]++;
    access_ctr++;
    
    if (curr_shift != last_shift) { // if current shift == last shift, there's no need to move anything
        if ((access_ctr % REMAP_INTERVAL == 0) && (map_ptr < NUM_SET)) // if all sets have been remapped, skip this step
            move_cache_data(lower_level);
    }

    if (access_ctr >= INTERVAL_SIZE) {
        cout << endl << endl;
        cout << "========================================================================" << endl;
        cout << "MODULE_ID " << MODULE_ID << ": Interval # " << num_interval << " Summary" << endl;
        cout << "========================================================================" << endl;
        cout << "   map_ptr:    " << map_ptr << endl;
        cout << "   accesses:   " << access_ctr << endl;
        cout << "   last_shift: " << last_shift << endl;
        cout << "   this_shift: " << curr_shift << endl;
        cout << "   next_shift: " << next_shift << endl;
        num_interval++;

        // get new shift setting for next interval
        last_shift = curr_shift;
        curr_shift = next_shift;
        next_shift = get_prediction();
        // uint32_t dummy = get_prediction();
        
        // Reset the runtime counters
        memset(N_ACCESS, 0, NUM_SET * sizeof(*N_ACCESS));
        memset(D_ACCESS, 0, NUM_SET * sizeof(*D_ACCESS));
        memset(EVICTION, 0, NUM_SET * sizeof(*EVICTION));
        map_ptr    = 0;
        access_ctr = 0;
    }
}


//========================================================================//
// CEASER-based gradual remapping scheme
void NeuralIndex::move_cache_data(MEMORY *lower_level) {

    for (uint32_t way=0; way < NUM_WAY; ++way) {
        BLOCK *block = &block_array[map_ptr][way];

        // initiate a move if the source block is valid
        if (block->valid) {
            
            // compute new index 
            uint32_t index_bits = block->address & ((1 << WINDOW_SIZE*5) - 1);
            uint32_t dest_index = compute_index(index_bits, curr_shift);


            // if new index == map_ptr => this block is already in the new position => skip
            if (dest_index == map_ptr)
                continue;

            // sanity check
            uint32_t set_id     = get_set(block->address);
            uint32_t last_index = compute_index(index_bits, last_shift);
            if (set_id != map_ptr) {
                cout << "ERROR: set_id != map_ptr" << endl;
                cout << "address:  " << block->address << endl;
                cout << "cpu:      " << CPU_ID << endl;
                cout << "set_id:   " << set_id << endl;
                cout << "map_ptr:  " << map_ptr << endl;
                cout << "curr_sft: " << curr_shift << endl;
                cout << "last_sft: " << last_shift << endl;
                cout << "curr_idx: " << dest_index << endl;
                // uint32_t last_xor_mask = (block->address >> last_shift) & ((1 << XOR_MASK_SIZE) - 1);
                // uint32_t last_index    = (last_shift) ? index_bits ^ last_xor_mask : index_bits;
                cout << "last_idx: " << last_index << endl;
                assert(false);
            }

            bool fill_ready = true;
            uint32_t dest_way = get_lru_victim(dest_index);
            // if the destination block is dirty, write it back before overriding the data
            if (block_array[dest_index][dest_way].valid && block_array[dest_index][dest_way].dirty) {
                if (!block->dirty) {
                    // if the destination block is dirty but the source block is clean, drop the source block (do not fill)
                    num_drop++;
                    fill_ready = false;
                }
                else { // if both blocks are dirty, write back the destination block then move the source block to the destination
                    // if the lower level memory is full
                    if (lower_level->get_occupancy(2, block_array[dest_index][dest_way].address) == lower_level->get_size(2, block_array[dest_index][dest_way].address)) {
                        // lower level WQ is full, cannot replace this victim
                        lower_level->increment_WQ_FULL(block_array[dest_index][dest_way].address);
                        fill_ready = false;
                        assert(false);
                        // moving_done = false;
                    }
                    else {
                        PACKET writeback_packet;
                        writeback_packet.fill_level = FILL_LEVEL << 1;
                        writeback_packet.cpu = CPU_ID;
                        writeback_packet.address = block_array[dest_index][dest_way].address;
                        writeback_packet.full_addr = block_array[dest_index][dest_way].full_addr;
                        writeback_packet.data = block_array[dest_index][dest_way].data;
                        writeback_packet.instr_id = block->instr_id;
                        writeback_packet.ip = 0; // writeback does not have ip
                        writeback_packet.type = WRITEBACK;
                        writeback_packet.event_cycle = current_core_cycle[CPU_ID];
                        lower_level->add_wq(&writeback_packet);

                        // update extra writeback statistics
                        num_writeback++;
                    }
                }
            }

            // override destination block with source block
            // we need not to up-invalidate the data in the destination block since the cache is non-inclusive
            if (fill_ready) {
                override_block(block, &block_array[dest_index][dest_way]);
                lru_update(dest_index, dest_way);
            }
            
            // invalidate the source block
            block->valid   = 0;
            block->address = 0;

            // update extra read statistics
            num_read++;
        }
    }
    map_ptr++;
}


//========================================================================//
uint32_t NeuralIndex::get_prediction() {
    string reply = sendMessage("make prediction "+MODULE_ID);

    // Compute reward (difference btw. std_devs)
    double normal_mean = compute_mean(N_ACCESS, NUM_SET);
    double neural_mean = compute_mean(D_ACCESS, NUM_SET);   
    double normal_sd   = 100*compute_std_dev(N_ACCESS, NUM_SET, normal_mean);
    double neural_sd   = 100*compute_std_dev(D_ACCESS, NUM_SET, neural_mean);
    float reward       = 10*(normal_sd - neural_sd)/normal_sd;

    double total_evictions = double(compute_sum(EVICTION, NUM_SET));
    // double total_accesses  = double(compute_sum(N_ACCESS, NUM_SET));
    // float  reward          = (-10)*total_evictions/total_accesses;        // reward = (-1)*eviction_rate


    // Get the most fluctuated set access counters
    vector<uint64_t> indices = get_significant_indices(N_ACCESS, NUM_SET, normal_mean, STATE_LENGTH);

    // Sanity check
    assert(indices.size() == STATE_LENGTH);

    // Compute state (under normal index)
    double state_array[STATE_LENGTH];
    memset(state_array, 0, sizeof(state_array));

    if (int(normal_sd*100)) {
        for (uint32_t i=0; i < indices.size(); ++i) {
            state_array[i] = (N_ACCESS[indices[i]] - normal_mean) / normal_sd;  // Standardize set access counters
        }
    }

    // Send state vector to the prediction server (sending in chunks)
    for (uint32_t chunk=0; chunk < NUM_CHUNKS; ++chunk)
        reply = sendMessage(flatten(state_array, (chunk+1)*CHUNK_SIZE, chunk*CHUNK_SIZE));

    
    // Server replies the prediction i.e., action
    uint32_t action = stoi(reply);

    // Sanity check
    if (action > MAX_SHIFTS) {
        cout << "ERROR: next_shift > MAX_SHIFTS" << endl;
        cout << "MAX_SHIFTS: " << MAX_SHIFTS << endl;
        cout << "next_shift: " << action << endl; 
        assert(false);
    }

    // Add new sample for this interval (interval n)
    uint32_t curr_index = (num_interval-1) % SAMPLE_BUFFER_SIZE;
    memcpy(sample_buffer[curr_index].state, state_array, STATE_LENGTH * sizeof(*(sample_buffer[curr_index].state)));
    sample_buffer[curr_index].action = action;

    
    // Update and send sample of (interval n-2)
    if (num_interval > 2) {
        uint32_t update_index = num_interval % SAMPLE_BUFFER_SIZE;
        memcpy(sample_buffer[update_index].next_state, state_array, STATE_LENGTH * sizeof(*(sample_buffer[update_index].next_state)));
        sample_buffer[update_index].reward = reward;
        sendSample(&(sample_buffer[update_index]));
    } 

    cout << "   normal sd:  " << normal_sd << endl;
    cout << "   neural_sd:  " << neural_sd << endl;
    cout << "   evictions:  " << total_evictions << endl;
    cout << "   reward:     " << reward << endl;
    cout << "   new_action: " << action << endl;
    cout << "========================================================================" << endl;

    // Send retrain signal
    if (num_interval > 2)
        retrain();

    return action;
}


//========================================================================//
void NeuralIndex::retrain() {
    string reply = sendMessage("retrain "+MODULE_ID);
    cout << "Num retrains: " << reply << endl;
}


//========================================================================//
uint32_t NeuralIndex::get_set(uint64_t address) {
    // determine current index and last index (based on current mask and last mask)
    uint32_t index_bits = address & ((1 << WINDOW_SIZE*5) - 1);
    uint32_t curr_index = compute_index(index_bits, curr_shift);
    uint32_t last_index = compute_index(index_bits, last_shift);

    // if last index >= map_ptr, this set has not yet been moved
    if (last_index >= map_ptr)
        return last_index;
    else
        return curr_index;
}

uint32_t NeuralIndex::compute_index(uint32_t index_bits, uint32_t mask_idx) {
    // compute 5 low bits
    uint32_t low_bits = (index_bits >> (WINDOW_SIZE*encoded_masks[mask_idx].lo)) & ((1 << WINDOW_SIZE) - 1);

    // compute 5 middle bits
    uint32_t mid_bits = (index_bits >> (WINDOW_SIZE*encoded_masks[mask_idx].mi)) & ((1 << WINDOW_SIZE) - 1);

    // compute 5 high bits
    uint32_t high_bits = (index_bits >> (WINDOW_SIZE*encoded_masks[mask_idx].hi)) & ((1 << WINDOW_SIZE) - 1);

    uint32_t index = (high_bits << 10) | (mid_bits << 5) | low_bits;

    // sanity check
    if (index >= NUM_SET) {
        cout << "Invalid set index: " << index << endl;
        assert(false);
    }

    if (mask_idx)
        index ^= (index_bits & ((1 << lg2(NUM_SET)) - 1));

    return index; 
}

//========================================================================//
void NeuralIndex::print_stats() {
    cout << "========================================================================" << endl;
    cout << "MODULE_ID:  " << MODULE_ID << endl;
    cout << "Extra WB:   " << num_writeback << endl;
    cout << "Extra READ: " << num_read << endl;
    cout << "Total DROP: " << num_drop << endl;
    cout << "========================================================================" << endl;
}


//========================================================================//
string NeuralIndex::sendMessage(string msg) {
    send(m_sock, msg.c_str(), strlen(msg.c_str()), 0);
    return getReply();
}


//========================================================================//
string NeuralIndex::getReply() {
    char buffer[m_bsize];
    int32_t valread = read(m_sock, buffer, m_bsize);
    string reply;
    for (int32_t i=0; i < valread; ++i) 
        reply += buffer[i];
    return reply;
}


//========================================================================//
void NeuralIndex::sendSample(Sample* sample) {
    string reply = sendMessage("new sample "+MODULE_ID);

    for (uint32_t chunk=0; chunk < NUM_CHUNKS; ++chunk)
        reply = sendMessage(flatten(sample->state, (chunk+1)*CHUNK_SIZE, chunk*CHUNK_SIZE));
    
    for (uint32_t chunk=0; chunk < NUM_CHUNKS; ++chunk)
        reply = sendMessage(flatten(sample->next_state, (chunk+1)*CHUNK_SIZE, chunk*CHUNK_SIZE));

    reply = sendMessage(to_string(sample->action));
    reply = sendMessage(to_string(sample->reward));    
}


//========================================================================//
// STATIC FUNCTIONS
//========================================================================//
void NeuralIndex::init_connection(uint32_t buffer_size) {
    m_bsize  = buffer_size;
    m_port   = readPortConfig(CONFIG_FILE);
    m_ipaddr = readIPConfig(CONFIG_FILE);
    m_sock   = createSocket();
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(m_port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, m_ipaddr.c_str(), &serv_addr.sin_addr) <= 0)
        throw "Invalid address / Address not supported";
    if (connect(m_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        throw "Connection Failed";
}

void NeuralIndex::close_connection(vector<string> models) {
    // Ending episode statistics
    cout << "========================================================================" << endl;
    cout << "DQN MODULE STATISTICS" << endl;
    cout << "========================================================================" << endl;
    for (uint32_t i=0; i < models.size(); ++i) {
        string reply   = sendMessage("disconnect "+models[i]);
        string eps     = "";
        string reward  = "";
        bool is_reward = false;
        for (uint32_t i=0; i < reply.size(); ++i) {
            if (reply[i] == '_') {
                is_reward = true;
                continue;
            }
            if (!is_reward)
                eps += reply[i];
            else
                reward += reply[i];
        }
        cout << "MODEL:      " << models[i] << endl;
        cout << "EPS:        " << eps << endl;
        cout << "Rewards:    " << reward << endl << endl;
    }
    cout << "========================================================================" << endl;
    close(m_sock);
}


//========================================================================//
// REPLACEMENT FUNCTIONS
//========================================================================//
uint32_t NeuralIndex::get_lru_victim(uint32_t set) {
    uint32_t way = 0;

    // fill invalid line first
    for (way=0; way<NUM_WAY; way++) {
        if (block_array[set][way].valid == false)
            break;
    }
    
    // LRU victim
    if (way == NUM_WAY) {
        for (way=0; way<NUM_WAY; way++) {
            if (block_array[set][way].lru == NUM_WAY-1)
                break;
        }
    }
    
    if (way >= NUM_WAY) {
        cout << "ERROR: way_id out of bound = " << way << endl;
        assert(false);
    }
    return way;
}

void NeuralIndex::lru_update(uint32_t set, uint32_t way) {
    // update lru replacement state
    for (uint32_t i=0; i<NUM_WAY; i++) {
        if (block_array[set][i].lru < block_array[set][way].lru) {
            block_array[set][i].lru++;
        }
    }
    block_array[set][way].lru = 0; // promote to the MRU position
}

void NeuralIndex::init_action_table() {
    encoded_masks.push_back(Action(0,1,2));
    // encoded_masks.push_back(Action(0,1,3));
    // encoded_masks.push_back(Action(0,1,4));
    // encoded_masks.push_back(Action(0,2,1));
    // encoded_masks.push_back(Action(0,2,3));
    // encoded_masks.push_back(Action(0,2,4));
    // encoded_masks.push_back(Action(0,3,1));
    // encoded_masks.push_back(Action(0,3,2));
    // encoded_masks.push_back(Action(0,3,4));
    // encoded_masks.push_back(Action(0,4,1));
    // encoded_masks.push_back(Action(0,4,2));
    // encoded_masks.push_back(Action(0,4,3));

    // encoded_masks.push_back(Action(1,0,2));
    encoded_masks.push_back(Action(1,0,3));
    encoded_masks.push_back(Action(1,0,4));
    encoded_masks.push_back(Action(1,2,0));
    encoded_masks.push_back(Action(1,2,3));
    encoded_masks.push_back(Action(1,2,4));
    encoded_masks.push_back(Action(1,3,0));
    // encoded_masks.push_back(Action(1,3,2));
    encoded_masks.push_back(Action(1,3,4));
    encoded_masks.push_back(Action(1,4,0));
    encoded_masks.push_back(Action(1,4,2));
    encoded_masks.push_back(Action(1,4,3));

    encoded_masks.push_back(Action(2,0,1));
    encoded_masks.push_back(Action(2,0,3));
    encoded_masks.push_back(Action(2,0,4));
    // encoded_masks.push_back(Action(2,1,0));
    // encoded_masks.push_back(Action(2,1,3));
    // encoded_masks.push_back(Action(2,1,4));
    encoded_masks.push_back(Action(2,3,0));
    encoded_masks.push_back(Action(2,3,1));
    encoded_masks.push_back(Action(2,3,4));
    encoded_masks.push_back(Action(2,4,0));
    encoded_masks.push_back(Action(2,4,1));
    encoded_masks.push_back(Action(2,4,3));

    encoded_masks.push_back(Action(3,0,1));
    // encoded_masks.push_back(Action(3,0,2));
    encoded_masks.push_back(Action(3,0,4));
    // encoded_masks.push_back(Action(3,1,0));
    // encoded_masks.push_back(Action(3,1,2));
    // encoded_masks.push_back(Action(3,1,4));
    encoded_masks.push_back(Action(3,2,0));
    encoded_masks.push_back(Action(3,2,1));
    encoded_masks.push_back(Action(3,2,4));
    encoded_masks.push_back(Action(3,4,0));
    encoded_masks.push_back(Action(3,4,1));
    // encoded_masks.push_back(Action(3,4,2));

    encoded_masks.push_back(Action(4,0,1));
    encoded_masks.push_back(Action(4,0,2));
    encoded_masks.push_back(Action(4,0,3));
    // encoded_masks.push_back(Action(4,1,0));
    // encoded_masks.push_back(Action(4,1,2));
    // encoded_masks.push_back(Action(4,1,3));
    encoded_masks.push_back(Action(4,2,0));
    encoded_masks.push_back(Action(4,2,1));
    encoded_masks.push_back(Action(4,2,3));
    encoded_masks.push_back(Action(4,3,0));
    encoded_masks.push_back(Action(4,3,1));
    encoded_masks.push_back(Action(4,3,2));
}
