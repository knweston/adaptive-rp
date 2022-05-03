#ifndef NEURAL_REPL_H
#define NEURAL_REPL_H

#include "cache_global.h"

#define MAX_SAMPLES 128
#define AD_SIG_LENGTH 32
#define IP_SIG_LENGTH 20

struct Signature {
    uint64_t addr = 0;
    uint64_t ip   = 0;
};

class NeuralRepl {
public:
    NeuralRepl();
    ~NeuralRepl();

    void add_sample(uint64_t ip, uint64_t addr);

    // server connection
    static void   init_connection(uint32_t buffer_size=8192);
    static void   close_connection(vector<string> models);
    static string sendMessage(string msg);
    static string getReply();
    void send_sample();
    bool get_prediction(uint64_t address, uint64_t ip);

public:
    // connection config
    static int32_t m_port;
    static int32_t m_sock;
    static int32_t m_bsize;
    static string  m_ipaddr;

    // counters
    bool hitmap[LLC_SET][LLC_WAY];
    uint64_t ip_table[LLC_SET][LLC_WAY];

    Signature* sample_buffer[MAX_SAMPLES];
    uint32_t sptr;
};

#endif // NEURAL_REPL_H