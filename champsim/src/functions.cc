#include "cache_global.h"
#include <fstream>
#include <math.h>
#include <algorithm>

//========================================================================//
// HELPER FUNCTIONS
//========================================================================//
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

double compute_mean(uint64_t* array, uint32_t array_size) {
    if (array_size)
        return double(compute_sum(array, array_size)) / array_size;
    else
        return 0;
}

double compute_std_dev(uint64_t* array, uint32_t array_size, double mean) {
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

void override_block(BLOCK *src, BLOCK *des) {
    des->valid      = src->valid;
    des->prefetch   = src->prefetch;
    des->dirty      = src->dirty;
    des->used       = src->used;
    des->delta      = src->delta;
    des->depth      = src->depth;
    des->signature  = src->signature; 
    des->confidence = src->confidence;
    des->address    = src->address;
    des->full_addr  = src->full_addr;
    des->tag        = src->tag;
    des->data       = src->data;
    des->ip         = src->ip;
    des->cpu        = src->cpu;
    des->instr_id   = src->instr_id; 
}

vector<uint64_t> get_significant_indices(uint64_t* array, uint32_t array_size, double mean, uint32_t num_select) {
    vector<Pair> pairs;
    for (uint32_t i=0; i < array_size; ++i) {
        uint64_t difference = abs(int(array[i] - mean));
        pairs.push_back(Pair(i, difference));
    }

    sort(pairs.begin(), pairs.end(), Pair::cmp_val_b);

    vector<uint64_t> top;
    for (uint32_t i=0; i < num_select; ++i)
        top.push_back(pairs[i].val_a);

    sort(top.begin(), top.end());
    return top;
}

uint64_t compute_sum(uint64_t* array, uint32_t array_size) {
    double total_accesses = 0.0;
    for (uint32_t i=0; i < array_size; ++i)
        total_accesses += array[i];
    return total_accesses;
}