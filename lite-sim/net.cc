#include "Net.h"
#include <assert.h>
#include <math.h>

//================================================================================================//
// STATIC VARIABLES
//================================================================================================//
double Net::m_recent_avg_smoothing_factor = 100.0; // Number of training samples to average over
double Neuron::eta = 0.15;  // overall net learning rate, [0.0..1.0]
double Neuron::alpha = 0.5; // momentum, multiplier of last deltaWeight, [0.0..1.0]

//================================================================================================//
// CLASS NET
//================================================================================================//
Net::Net(std::vector<unsigned> &topology) {
    unsigned num_layers = topology.size();
    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        m_layers.push_back(Layer());
        unsigned num_outputs = layer_num == topology.size()-1 ? 0 : topology[layer_num+1];
        for (int neuron_num = 0; neuron_num <= topology[layer_num]; ++neuron_num) {
            m_layers.back().push_back(Neuron(num_outputs, neuron_num));
        }
        
        // Force the bias node's output to 1.0 (it was the last neuron pushed in this layer):
        m_layers.back().back().setOutputVal(1.0);
    }
}
    

void Net::feedForward(std::vector<double> &input_vals) {
    assert(input_vals.size() == m_layers[0].size()-1);

    // Assign input values into the input neurons
    for (int i=0; i < input_vals.size(); ++i) {
        m_layers[0][i].setOutputVal(input_vals[i]);
    }

    // Forward propagate (starting from the 1st hidden layer)
    for (int layer_num = 1; layer_num < m_layers.size(); ++layer_num) {
        Layer &prev_layer = m_layers[layer_num-1];
        for (int n=0; n < m_layers[layer_num].size()-1; ++n) {
            m_layers[layer_num][n].feedForward(prev_layer);
        }
    }
}

void Net::backProp(std::vector<double> &target_vals) {
    // Calculate overall net error (RMS of output neuron errors)
    Layer &output_layer = m_layers.back();
    m_error = 0.0;
    
    for (int n=0; n < output_layer.size()-1; ++n) {
        double delta = target_vals[n] - output_layer[n].getOutputVal();
        m_error += delta * delta;
    }
    m_error /= output_layer.size()-1;
    m_error = sqrt(m_error);

    // Implement a recent average measurement
    m_recent_avg_error = (m_recent_avg_error * m_recent_avg_smoothing_factor + m_error)
                         / (m_recent_avg_smoothing_factor + 1.0);

    // Calculate output layer gradients
    for (int n=0; n < output_layer.size()-1; ++n) {
        output_layer[n].calcOutputGradients(target_vals[n]);
    }

    // Calculate gradients on hidden layers
    for (int layer_num = m_layers.size()-2; layer_num > 0; --layer_num) {
        Layer &hidden_layer = m_layers[layer_num];
        Layer &next_layer   = m_layers[layer_num+1];

        for (int n=0; n < hidden_layer.size(); ++n) {
            hidden_layer[n].calcHiddenGradients(next_layer);
        }
    }
    
    // Update connection weights of all layers except the input layer
    for (int layer_num = m_layers.size()-1; layer_num > 0; --layer_num) {
        Layer &layer = m_layers[layer_num];
        Layer &prev_layer = m_layers[layer_num-1];

        for (int n = 0; n < layer.size() - 1; ++n) {
            layer[n].updateInputWeights(prev_layer);
        }
    }
}

void Net::getResults(std::vector<double> &result_vals) {
    result_vals.clear();
    for (unsigned n = 0; n < m_layers.back().size() - 1; ++n) {
        result_vals.push_back(m_layers.back()[n].getOutputVal());
    }
}


//================================================================================================//
// CLASS NEURON
//================================================================================================//
Neuron::Neuron(unsigned num_outputs, unsigned my_index) {
    for (int i = 0; i < num_outputs; ++i) {
        m_output_weights.push_back(Connection());
        m_output_weights.back().weight = randomWeight();
    }
    m_my_index = my_index;
}


void Neuron::feedForward(Layer &prev_layer) {
    double sum = 0.0;

    // Input of this neuron = sum of prev_layer's outputs including bias node
    for (int n=0; n < prev_layer.size(); ++n) {
        sum += prev_layer[n].getOutputVal() * prev_layer[n].m_output_weights[m_my_index].weight;
    } 
    m_output_val = Neuron::activation(sum, "tanh");
}


void Neuron::calcOutputGradients(double target_val) {
    double delta = target_val - m_output_val;
    m_gradient = delta * Neuron::activationDerivative(m_output_val, "tanh");
}


void Neuron::calcHiddenGradients(Layer &next_layer) {
    double dow = sumDOW(next_layer);
    m_gradient = dow * Neuron::activationDerivative(m_output_val, "tanh");
}


void Neuron::updateInputWeights(Layer &prev_layer) {
    // The weights to be updated are in the Connection container
    // in the neurons in the preceding layer (i.e optimizer function)
    for (unsigned n = 0; n < prev_layer.size(); ++n) {
        Neuron &neuron = prev_layer[n];
        double old_delta_weight = neuron.m_output_weights[m_my_index].delta_weight;

        // Individual input, magnified by the gradient and learning rate
        // Also add momentum = a fraction of the previous delta weight
        double new_delta_weight = eta * neuron.getOutputVal() * m_gradient + alpha * old_delta_weight;
                
        neuron.m_output_weights[m_my_index].delta_weight = new_delta_weight;
        neuron.m_output_weights[m_my_index].weight += new_delta_weight;
    }
}


double Neuron::sumDOW(const Layer &next_layer) const {
    double sum = 0.0;

    // Sum our contributions of the errors at the nodes we feed (Differential of Weights)
    for (unsigned n = 0; n < next_layer.size() - 1; ++n) {
        sum += m_output_weights[n].weight * next_layer[n].m_gradient;
    }

    return sum;
}


double Neuron::activation(double input, string type) {
    if (type == "tanh")         // output = [-1.0, 1.0]
        return tanh(input); 
    else if (type == "sigmoid") // output = [0.0, 1.0]
        return sigmoid(input);
    else {
        cout << "Unrecognized activation(): " << type << endl;
        assert(false);
    }
}

double Neuron::activationDerivative(double input, string type) {
    if (type == "tanh")
        return (1.0 - input*input); // Derivative of tanh
    else if (type == "sigmoid")
        return sigmoid(input) * (1 - sigmoid(input));
    else {
        cout << "Unrecognized activation_deriv(): " << type << endl;
        assert(false);
    }
}

double sigmoid(double x) {
     double result;
     result = 1 / (1 + exp(-x));
     return result;
}