#ifndef NET_H
#define NET_H

#include "global.h"

//====================================================================================//
// FORWARD DECLARATIONS - TYPEDEF - CONTAINERS
//====================================================================================//
class Neuron;
struct Layer {
    Layer() {};
    Layer(string a)
        :activation(a){}
    std::vector<Neuron> neuron_array;
    string activation; 
};

struct Connection {
    double weight;
    double delta_weight;
};


//====================================================================================//
// CLASS NET
//====================================================================================//
class Net {
public:
    Net(std::vector<unsigned> &topology);
    void feedForward(std::vector<double> &input_vals);
    double backProp(std::vector<double> &target_vals);
    void getResults(std::vector<double> &result_vals);
    double getRecentAverageError() const { return m_recent_avg_error; }
    void fit(vector<vector<double>>& x_train, vector<vector<double>>& y_train, uint32_t epochs=1);
    uint32_t predict(uint64_t addr, uint64_t ip);

    double compute_loss(std::vector<double> &target_vals, string type="rms");

private:
    std::vector<Layer> m_layers;
    double m_error;
    double m_recent_avg_error;
    static double m_recent_avg_smoothing_factor;
};


//====================================================================================//
// CLASS NEURON
//====================================================================================//
class Neuron {
public:
    Neuron(unsigned num_outputs, unsigned my_index);
    void feedForward(Layer &prev_layer, string act_type);
    void setOutputVal(double val) { m_output_val = val; }
    double getOutputVal() { return m_output_val; }
    void calcOutputGradients(double target_val, string act_type);
    void calcHiddenGradients(Layer &next_layer, string act_type);
    void updateInputWeights(Layer &prev_layer);

private:
    double m_output_val;
    unsigned m_my_index;
    double m_gradient;
    std::vector<Connection> m_output_weights;
    double sumDOW(const Layer &next_layer) const;
    
    static double alpha;
    static double gamma;
    static double randomWeight() { return rand() / double(RAND_MAX); }
    static double activation(double input, string type);
    static double activationDerivative(double input, string type);
};

double sigmoid(double x);
double huber(double x, double delta=1.0);
double relu(double x);
double relu_deriv(double x);

#endif // NET_H