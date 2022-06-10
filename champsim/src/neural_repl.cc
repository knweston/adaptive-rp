#include "neural_repl.h"

//========================================================================//
// STATIC PARAMETERS
//========================================================================//
int32_t NeuralRepl::m_port;
int32_t NeuralRepl::m_sock;
int32_t NeuralRepl::m_bsize;
string  NeuralRepl::m_ipaddr;

//========================================================================//
// STATIC FUNCTIONS
//========================================================================//
void NeuralRepl::init_connection(uint32_t buffer_size) {
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

void NeuralRepl::close_connection(vector<string> models) {
    // Ending episode statistics
    cout << "========================================================================" << endl;
    cout << "ML MODULE STATISTICS" << endl;
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
string NeuralRepl::sendMessage(string msg) {
    send(m_sock, msg.c_str(), strlen(msg.c_str()), 0);
    return getReply();
}

//========================================================================//
string NeuralRepl::getReply() {
    char buffer[m_bsize];
    int32_t valread = read(m_sock, buffer, m_bsize);
    string reply;
    for (int32_t i=0; i < valread; ++i) 
        reply += buffer[i];
    return reply;
}

//========================================================================//
// MEMBER FUNCTIONS
//========================================================================//
NeuralRepl::NeuralRepl() {
    for (uint32_t set=0; set < LLC_SET; ++set) {
        for (uint32_t way=0; way < LLC_WAY; ++way) {
            hitmap[set][way]   = 0;
            ip_table[set][way] = 0;
        }
    }

    for (uint32_t i=0; i < MAX_SAMPLES; ++i)
        sample_buffer[i] = new Signature();

}

NeuralRepl::~NeuralRepl() {
    for (uint32_t i=0; i < MAX_SAMPLES; ++i)
        delete sample_buffer[i];
}

void NeuralRepl::add_sample(uint64_t ip, uint64_t addr) {
    sample_buffer[sptr]->ip   = ip & ((1 << IP_SIG_LENGTH) - 1);
    sample_buffer[sptr]->addr = addr & (((uint64_t)(1) << AD_SIG_LENGTH) - 1);
    ++sptr;
    if (sptr == MAX_SAMPLES) {
        // TODO: send samples
        sptr = 0;
    }
}

void NeuralRepl::send_sample() {
    string reply = "";
    string data  = "new sample_";

    for (uint32_t i=0; i < MAX_SAMPLES; ++i)
        data += to_string(sample_buffer[i]->ip) + "_" + to_string(sample_buffer[i]->addr) + "_";
    data.pop_back();
    reply = sendMessage(data);
}

bool NeuralRepl::get_prediction(uint64_t address, uint64_t ip) {
    string data  = "make prediction_" + to_string(ip) + "_" + to_string(address);
    string reply = sendMessage(data);

    if (reply == "0")
        return false;
    else
        return true;
}