#include <vector>
#include <stdlib.h>

//====================================================================================//
// FORWARD DECLARATIONS - TYPEDEF - CONTAINERS
//====================================================================================//
class Neuron;
typedef std::vector<Neuron> Layer;

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
    void backProp(std::vector<double> &target_vals);
    void getResults(std::vector<double> &result_vals);
    double getRecentAverageError() const { return m_recent_avg_error; }

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
    void feedForward(Layer &prev_layer);
    void setOutputVal(double val) { m_output_val = val; }
    double getOutputVal() { return m_output_val; }
    void calcOutputGradients(double target_val);
    void calcHiddenGradients(Layer &next_layer);
    void updateInputWeights(Layer &prev_layer);

private:
    double m_output_val;
    unsigned m_my_index;
    double m_gradient;
    std::vector<Connection> m_output_weights;
    double sumDOW(const Layer &next_layer) const;
    
    static double eta;
    static double alpha;
    static double randomWeight() { return rand() / double(RAND_MAX); }
    static double activation(double input);
    static double activationDerivative(double input);
};

double sigmoid(double x);