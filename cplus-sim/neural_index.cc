#include "neural_index.h"

//========================================================================//
// CLASS NEURAL_INDEX
//========================================================================//
NeuralIndex::NeuralIndex(int32_t buffer_size) {
    m_port   = readPortConfig(CONFIG_FILE);
    m_ipaddr = readIPConfig(CONFIG_FILE);
    m_bsize  = buffer_size;
    m_sock   = createSocket();

    curr_shifts = 0;
    next_shifts = 0;
    num_intervals = 0;
    
    for (uint32_t i=0; i < SAMPLE_BUFFER_SIZE; ++i)
        sample_buffer[i] = new Sample();

    connectServer(m_port, m_sock, m_ipaddr);
}

//========================================================================//
NeuralIndex::~NeuralIndex() {
    for (uint32_t i=0; i < SAMPLE_BUFFER_SIZE; ++i) {
        delete sample_buffer[i];
    }
    disconnectServer(m_sock, m_bsize);
}

void NeuralIndex::update(uint64_t address) {
    uint32_t neural_set = get_set(address);
    uint32_t normal_set = address & ((1 << lg2(LLC_SET)) - 1);
    N_ACCESS[normal_set]++;
    D_ACCESS[neural_set]++;

    if ((CURRENT_PTR % INTERVAL_SIZE == 0) && (CURRENT_PTR > 0)) {
        cout << "shift_(n):   " << curr_shifts << endl;
        cout << "shift_(n+1): " << next_shifts << endl;
        num_intervals++;

        curr_shifts = next_shifts;
        get_prediction();
        move_cache_data();
        
        // Reset the runtime counters
        memset(N_ACCESS, 0, sizeof(N_ACCESS));
        memset(D_ACCESS, 0, sizeof(D_ACCESS));
    }
    
}


// move current cache data to new places based on the new mapping
// rule: first come first serve, blocks coming late are dropped
void NeuralIndex::move_cache_data() {
    uint64_t mirrored_cache[LLC_SET][LLC_WAY];
    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way)
            mirrored_cache[set][way] = 0;
    }

    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way) {
            uint32_t set_index = get_set(CACHE[set][way]);
            for (uint32_t i=0; i < LLC_WAY; ++i) {
                if (mirrored_cache[set_index][i] == 0)
                    mirrored_cache[set_index][i] = CACHE[set][way];
            }
        }
    }
    
    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way)
            CACHE[set][way] = mirrored_cache[set][way];
    }
}


//========================================================================//
void NeuralIndex::get_prediction() {
    string reply = sendMessage(m_sock, m_bsize, "make prediction");

    // Compute reward (difference btw. std_devs)
    double neural_sd = 100*compute_std_dev(D_ACCESS, STATE_LENGTH);
    double normal_sd = 100*compute_std_dev(N_ACCESS, STATE_LENGTH);
    int32_t reward = int(100*(normal_sd - neural_sd)/normal_sd);

    // Compute state (under normal index)
    double state_array[STATE_LENGTH];
    double mean = compute_mean(N_ACCESS, STATE_LENGTH);

    // Normalize access counter
    for (uint32_t i=0; i < STATE_LENGTH; ++i) {
        if (int(normal_sd*100))
            state_array[i] = (N_ACCESS[i] - mean) / normal_sd;
        else
            state_array[i] = 0;
    }
    
    // Send state vector to the prediction server (sending in two chunks)
    reply = sendMessage(m_sock, m_bsize, flatten(state_array, STATE_LENGTH/2, 0));
    reply = sendMessage(m_sock, m_bsize, flatten(state_array, STATE_LENGTH, STATE_LENGTH/2));
    
    // Server replies the prediction i.e., action
    uint32_t action = stoi(reply);

    // Update next XOR-mask and num shifting windows
    next_shifts = action;
    
    // Add new sample for this interval (interval n)
    uint32_t curr_index = (num_intervals-1) % SAMPLE_BUFFER_SIZE;
    memcpy(sample_buffer[curr_index]->state, state_array, sizeof(sample_buffer[curr_index]->state));
    sample_buffer[curr_index]->action = action;

    // Update and send sample of (interval n-2)
    if (num_intervals > 2) {
        uint32_t update_index = num_intervals % SAMPLE_BUFFER_SIZE;
        memcpy(sample_buffer[update_index]->next_state, state_array, sizeof(sample_buffer[update_index]->next_state));
        sample_buffer[update_index]->reward = reward;
        sendSample(sample_buffer[update_index]);
    } 

    cout << "shift_(n+2): " << action << endl;
    cout << "normal sd:   " << normal_sd << endl;
    cout << "neural_sd:   " << neural_sd << endl;
    cout << "reward:      " << reward << endl;
    cout << endl;

    // Send retrain signal
    if (num_intervals > 2)
        retrain();
}

//========================================================================//
void NeuralIndex::retrain() {
    string reply = sendMessage(m_sock, m_bsize, "retrain");
    cout << "Model retrained successfully" << endl;
}

//========================================================================//
uint32_t NeuralIndex::get_set(uint64_t address) {
    uint32_t shorten_addr = address & ((1 << lg2(LLC_SET)) - 1);
    uint32_t xor_mask = (address >> curr_shifts) & ((1 << XOR_MASK_SIZE) - 1);

    if (curr_shifts)
        return shorten_addr ^ xor_mask;
    else
        return shorten_addr;
    
}

//========================================================================//
void NeuralIndex::sendSample(Sample* sample) {
    string reply = sendMessage(m_sock, m_bsize, "new sample");
    reply        = sendMessage(m_sock, m_bsize, flatten(sample->state, STATE_LENGTH/2, 0));
    reply        = sendMessage(m_sock, m_bsize, flatten(sample->state, STATE_LENGTH, STATE_LENGTH/2));
    reply        = sendMessage(m_sock, m_bsize, flatten(sample->next_state, STATE_LENGTH/2, 0));
    reply        = sendMessage(m_sock, m_bsize, flatten(sample->next_state, STATE_LENGTH, STATE_LENGTH/2));
    reply        = sendMessage(m_sock, m_bsize, to_string(sample->action));
    reply        = sendMessage(m_sock, m_bsize, to_string(sample->reward));
}