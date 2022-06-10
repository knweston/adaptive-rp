#include "global.h"
#include "net.h"

//========================================================================================================================//
// GLOBAL VARIABLES
//========================================================================================================================//
vector<Access> GLOBAL_HISTORY;
uint64_t CACHE[LLC_SET][LLC_WAY];

uint64_t ACCESSES[NUM_TYPES];
uint64_t HITS[NUM_TYPES];
uint64_t MISSES[NUM_TYPES];
uint64_t EVICTIONS[LLC_SET];

uint64_t BYPASSES          = 0;
uint32_t NUM_THREADS     = LLC_WAY > MAX_THREADS ? MAX_THREADS : LLC_WAY;
uint32_t PRINT_INTERVAL  = 0;
uint64_t CURRENT_PTR     = 0;
uint32_t CHECKPOINT      = 0;
string   TRACE_FILE      = "";
string   INDEXING_SCHEME = "NORMAL";
string   REPL_POLICY     = "LRU";
string   CONFIG_FILE     = "";

//========================================================================================================================//
// MAIN FUNCTION
//========================================================================================================================//
void int_to_bin(uint32_t integer, vector<double>& bin_array) {
    for (uint32_t i=0; i < ADDR_LENGTH; ++i)
        bin_array[ADDR_LENGTH-1-i] = (integer >> i) & 1;
}

int main(int argc, char *argv[]) {
    TRACE_FILE = argv[1];
    init_global_history();
    uint32_t NUM_INPUTS = 32;
    vector<uint32_t> topology = {NUM_INPUTS, NUM_INPUTS, NUM_INPUTS};
    Net my_net = Net(topology);

    for (uint32_t times=0; times < 35; ++times) {
        vector<vector<double>> x_train; 
        vector<double> sample(NUM_INPUTS, 0);
        for (uint32_t n=1; n <= 512; ++n) {
            
            uint32_t sum = 0;
            for (uint32_t i=0; i < sample.size(); ++i) {
                if (sum < sample.size()/2)
                    sample[i] = rand() % 2;
                else
                    sample[i] = 0;
                ++sum;
                // sample[i] = rand() % 2;
            }

            x_train.push_back(sample);
        }

        cout << "Num samples: " << x_train.size() << endl;

        for (uint32_t epoch=0; epoch < 25; ++epoch) {
            double epoch_loss = 0.0;
            for (uint32_t sample=0; sample < x_train.size(); ++sample) {
                my_net.feedForward(x_train[sample]);
                vector<double> y_pred;
                my_net.getResults(y_pred);
                double loss = my_net.backProp(x_train[sample]);

                // cout << "Sample: " << sample << endl;
                // cout << "   x:    [";
                // for (uint32_t i=0; i < x_train[sample].size(); ++i) {
                //     if (i < x_train[sample].size()-1)
                //         cout << x_train[sample][i] << ",";
                //     else
                //         cout << x_train[sample][i];
                // }
                // cout << "]" << endl;

                // cout << "   pred: [";
                // for (uint32_t i=0; i < y_pred.size(); ++i) {
                //     if (i < y_pred.size()-1)
                //         cout << y_pred[i] << ",";
                //     else
                //         cout << y_pred[i];
                // }
                // cout << "]" << endl;
                // cout << "   Loss: " << loss << ". Recent AVG Error: " << my_net.getRecentAverageError() << endl;
                epoch_loss += loss;
            }
            cout << "Epoch loss: " << epoch_loss/x_train.size() << endl;
            // cout << "============================================================" << endl;
        }
    }

    return 0;
}
