#include "global.h"

//========================================================================================================================//
// HELPER CLASSES
//========================================================================================================================//
bool Record::compare_frequency(const Record &r1, const Record &r2) {
    if (r1.frequency == r2.frequency)
        return r1.address < r2.address;
    else
        return r1.frequency > r2.frequency;
}

//========================================================================================================================//
// HELPER FUNCTIONS
//========================================================================================================================//
void init_global_history() {
    ifstream trace_file(TRACE_FILE);

    if (trace_file) {
        while(!trace_file.eof()) {
            string line;
            uint64_t address=0;
            uint64_t ip=0;
            string type="";
            getline(trace_file, line);
            stringstream ss(line);
            ss >> address;
            ss >> ip;
            ss >> type;
            

            // skip zero address
            if (!address)
                continue;

            // add new access entry to the global access history
            Access new_access;
            new_access.address = address;
            new_access.ip      = ip;
            
            if (type == "LOAD")
                new_access.type = 0;
            else if (type == "RFO")
                new_access.type = 1;
            else if (type == "PREFETCH")
                new_access.type = 2;
            else if (type == "WRITEBACK")
                new_access.type = 3;
            else {
                cout << "Unrecognized Access Type: " << type << endl;
                exit(1);
            }
            GLOBAL_HISTORY.push_back(new_access);
        }
    }
}

int check_hit(uint64_t address, uint32_t set_index) {
    for (uint32_t way=0; way < LLC_WAY; ++way) {
        if (CACHE[set_index][way] == address)
            return way;
    }
    return -1;
}

void print_final_stats() {
    uint64_t total_accesses  = ACCESSES[LOAD] + ACCESSES[RFO] + ACCESSES[PREFETCH] + ACCESSES[WRITEBACK];
    uint64_t total_hits      = HITS[LOAD] + HITS[RFO] + HITS[PREFETCH] + HITS[WRITEBACK];
    uint64_t total_misses    = MISSES[LOAD] + MISSES[RFO] + MISSES[PREFETCH] + MISSES[WRITEBACK];
    uint64_t total_evictions = 0;
    for (uint32_t set=0; set < LLC_SET; ++set)
        total_evictions += EVICTIONS[set];

    cout << "TRACE   = " << TRACE_FILE << endl;
    cout << "LLC_SET = " << LLC_SET << endl;
    cout << "LLC_WAY = " << LLC_WAY << endl;
    cout << "THREADS = " << NUM_THREADS << endl;
    cout << "========================================================================" << endl;
    cout << "LLC FINAL STATISTICS" << endl;
    cout << "========================================================================" << endl;
    cout << left << setw(15) << ""           << setw(15) << "ACCESSES"          << setw(15) << "HITS"          << setw(15) << "MISSES"          << endl;
    cout << left << setw(15) << "LOAD"       << setw(15) << ACCESSES[LOAD]      << setw(15) << HITS[LOAD]      << setw(15) << MISSES[LOAD]      << endl;
    cout << left << setw(15) << "RFO"        << setw(15) << ACCESSES[RFO]       << setw(15) << HITS[RFO]       << setw(15) << MISSES[RFO]       << endl; 
    cout << left << setw(15) << "PREFETCH"   << setw(15) << ACCESSES[PREFETCH]  << setw(15) << HITS[PREFETCH]  << setw(15) << MISSES[PREFETCH]  << endl; 
    cout << left << setw(15) << "WRITEBACK"  << setw(15) << ACCESSES[WRITEBACK] << setw(15) << HITS[WRITEBACK] << setw(15) << MISSES[WRITEBACK] << endl;
    cout << "========================================================================" << endl;
    cout << left << setw(15) << "TOTAL"      << setw(15) << total_accesses      << setw(15) << total_hits      << setw(15) << total_misses << endl;
    cout << "========================================================================" << endl;
    cout << left << setw(15) << "EVICTIONS"  << total_evictions << endl;
    cout << "========================================================================" << endl;
}

// log base 2 function from efectiu (from ChampSim)
int lg2(int n) {
    int i, m = n, c = -1;
    for (i=0; m; i++) {
        m /= 2;
        c++;
    }
    return c;
}

int64_t search(vector<Record> &array, int64_t left_pt, int64_t right_pt, Record &element) {
    if (right_pt >= left_pt) {
        uint64_t mid_pt = left_pt + (right_pt - left_pt)/2;
        if (array[mid_pt] == element)
            return mid_pt;
        else if (array[mid_pt] > element)
            return search(array, left_pt, mid_pt-1, element);
        else
            return search(array, mid_pt+1, right_pt, element);
    }
    return -1;
}

void* compute_distance(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    for (uint32_t i=0; i < LLC_WAY/NUM_THREADS; ++i) {
        uint32_t way_id = data->way_list[i];
        uint64_t index  = data->start_pt;
        for (index; index < data->history->size(); ++index) {
            if (data->victim_set[way_id] == (*data->history)[index]) // address found
                break;
        }
        data->rdist_arr[way_id] = index - data->start_pt;
    }
    return 0;
}

void create_checkpoint(ofstream& cp_file) {
    // Save simulation settings
    cp_file << CURRENT_PTR << endl;
    cp_file << PRINT_INTERVAL << endl;
    cp_file << INDEXING_SCHEME << endl;
    cp_file << REPL_POLICY << endl;

    // Save performance counters' state
    for (uint32_t type=0; type < NUM_TYPES; ++type) {
        cp_file << ACCESSES[type] << endl;
        cp_file << HITS[type] << endl;
        cp_file << MISSES[type] << endl;
    }
    for (uint32_t set=0; set < LLC_SET; ++set)
        cp_file << EVICTIONS[set] << endl;

    // Save cache content
    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way)
            cp_file << CACHE[set][way] << endl;
    }
}

void load_checkpoint(ifstream& cp_file) {
    // Load simulation settings
    string line;
    getline(cp_file, line);
    CURRENT_PTR    = stol(line);
    getline(cp_file, line);
    PRINT_INTERVAL = stol(line);
    getline(cp_file, INDEXING_SCHEME);
    getline(cp_file, REPL_POLICY);

    // Load performance counters' content
    for (uint32_t type=0; type < NUM_TYPES; ++type) {
        getline(cp_file, line);
        ACCESSES[type] = stol(line);
        getline(cp_file, line);
        HITS[type] = stol(line);
        getline(cp_file, line);
        MISSES[type] = stol(line);
    }
    for (uint32_t set=0; set < LLC_SET; ++set) {
        getline(cp_file, line);
        EVICTIONS[set] = stol(line);
    }

    // Load cache content
    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way) {
            getline(cp_file, line);
            CACHE[set][way] = stol(line);
        }
    }
}

void export_eviction_result() {
    uint64_t total_evictions = 0;
    string filename;

    for (uint32_t i=0; i < TRACE_FILE.size()-13; ++i)
        filename += TRACE_FILE[i];
    filename += "SET-EVICTIONS.txt";
    ofstream out_file(filename);
    for (uint32_t set=0; set < LLC_SET; ++set)
        out_file << set << " " << EVICTIONS[set] << endl;
    out_file.close();
}

int32_t readPortConfig(string filename) {
    ifstream conf_file(filename);
    string line;
    int32_t port_num=-1;
    if (conf_file.is_open()) {
        while(getline(conf_file, line)) {
            if (line.find("port") != string::npos) {
                uint64_t split_index=0;
                for (; split_index < line.size(); ++split_index) {
                    if (line[split_index] == '=') {
                        split_index++;
                        break;
                    }
                }
                string temp;
                for (; split_index < line.size(); ++split_index) {
                    temp += line[split_index];
                }
                port_num = stoi(temp);
                break;
            }
        }
        conf_file.close();
    }
    return port_num;
}

string readIPConfig(string filename) {
    ifstream conf_file(filename);
    string line;
    string ip_address;
    if (conf_file.is_open()) {
        while(getline(conf_file, line)) {
            if (line.find("ipaddress") != string::npos) {
                uint64_t split_index=0;
                for (; split_index < line.size(); ++split_index) {
                    if (line[split_index] == '=') {
                        split_index++;
                        break;
                    }
                }
                for (; split_index < line.size(); ++split_index) {
                    if (line[split_index] != ' ')
                        ip_address += line[split_index];
                }
                break;
            }
        }
        conf_file.close();
    }
    return ip_address;
}

int32_t createSocket() {
    int32_t sock = 0;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw "Socket creation error";
    return sock;
}


//========================================================================//
void connectServer(int32_t port, int32_t sock, string ipaddr) {
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ipaddr.c_str(), &serv_addr.sin_addr) <= 0)
        throw "Invalid address / Address not supported";
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        throw "Connection Failed";
}

//========================================================================//
void disconnectServer(int32_t sock, int32_t bsize) {
    string reply   = sendMessage(sock, bsize, "disconnect");
    cout << reply << endl;
    close(sock); 
}

//========================================================================//
string sendMessage(int32_t sock, int32_t bsize, string msg) {
    send(sock, msg.c_str(), strlen(msg.c_str()), 0);
    return getReply(sock, bsize);
}

//========================================================================//
string getReply(int32_t sock, int32_t bsize) {
    char buffer[bsize];
    int32_t valread = read(sock, buffer, bsize);
    string reply;
    for (int32_t i=0; i < valread; ++i) 
        reply += buffer[i];
    return reply;
}

double compute_mean(uint64_t* array, uint64_t array_size) {
    if (array_size) {
        double total_accesses = 0.0;
        for (uint32_t i=0; i < array_size; ++i)
            total_accesses += array[i];
        return total_accesses / array_size;
    }
    else
        return 0;
}

double compute_std_dev(uint64_t* array, uint64_t array_size) {
    double mean     = compute_mean(array, array_size);
    double variance = 0.0;
    for (uint32_t i=0; i < array_size; ++i)
        variance += pow(array[i] - mean, 2);
    variance /= (array_size - 1);
    return sqrt(variance);
}

string flatten(double* array, uint64_t end_pt, uint64_t start_pt) {
    string data = "";
    for (uint32_t i=start_pt; i < end_pt; ++i)
        data += to_string(array[i]) + "_";

    data.pop_back();
    return data;
}

void reset_statistics() {
    memset(ACCESSES, 0, sizeof(ACCESSES));
    memset(HITS, 0, sizeof(HITS));
    memset(MISSES, 0, sizeof(MISSES));
    memset(EVICTIONS, 0, sizeof(EVICTIONS));
}