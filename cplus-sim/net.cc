#include "net.h"

//================================================================================================//
// STATIC VARIABLES
//================================================================================================//
double Net::m_recent_avg_smoothing_factor = 100.0; // Number of training samples to average over
double Neuron::alpha = 0.01;  // overall net learning rate, [0.0..1.0]
double Neuron::gamma = 0.5; // momentum, multiplier of last deltaWeight, [0.0..1.0]

//================================================================================================//
// CLASS NET
//================================================================================================//
Net::Net(std::vector<unsigned> &topology) {
    unsigned num_layers = topology.size();
    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        m_layers.push_back(Layer());
        unsigned num_outputs = layer_num == topology.size()-1 ? 0 : topology[layer_num+1];
        for (int neuron_num = 0; neuron_num <= topology[layer_num]; ++neuron_num) {
            m_layers.back().neuron_array.push_back(Neuron(num_outputs, neuron_num));
        }
        
        // Force the bias node's output to 1.0 (it was the last neuron pushed in this layer):
        m_layers.back().neuron_array.back().setOutputVal(1.0);
        if (layer_num == (num_layers-1))
            m_layers.back().activation = "sigmoid";
        else
            m_layers.back().activation = "relu";
    }

    for (uint32_t i=0; i<m_layers.size(); ++i)
        cout << "layer activation: " << m_layers[i].activation << endl;
}
    

void Net::feedForward(std::vector<double> &input_vals) {
    assert(input_vals.size() == m_layers[0].neuron_array.size()-1);

    // Assign input values into the input neurons
    for (int i=0; i < input_vals.size(); ++i)
        m_layers[0].neuron_array[i].setOutputVal(input_vals[i]);

    // Forward propagate (starting from the 1st hidden layer)
    for (int layer_num = 1; layer_num < m_layers.size(); ++layer_num) {
        Layer &prev_layer = m_layers[layer_num-1];      

        for (int n=0; n < m_layers[layer_num].neuron_array.size()-1; ++n)
            m_layers[layer_num].neuron_array[n].feedForward(prev_layer, m_layers[layer_num].activation);
    }

}

double Net::backProp(std::vector<double> &target_vals) {
    // Calculate overall net error (RMS of output neuron errors)
    m_error = compute_loss(target_vals, "huber");

    // Implement a recent average measurement
    m_recent_avg_error = (m_recent_avg_error * m_recent_avg_smoothing_factor + m_error)
                         / (m_recent_avg_smoothing_factor + 1.0);

    // Calculate output layer gradients
    Layer &output_layer = m_layers.back();
    for (int n=0; n < output_layer.neuron_array.size()-1; ++n) {
        output_layer.neuron_array[n].calcOutputGradients(target_vals[n], output_layer.activation);
    }

    // Calculate gradients on hidden layers
    for (int layer_num = m_layers.size()-2; layer_num > 0; --layer_num) {
        Layer &hidden_layer = m_layers[layer_num];
        Layer &next_layer   = m_layers[layer_num+1];

        for (int n=0; n < hidden_layer.neuron_array.size(); ++n) {
            hidden_layer.neuron_array[n].calcHiddenGradients(next_layer, hidden_layer.activation);
        }
    }
    
    // Update connection weights of all layers except the input layer
    for (int layer_num = m_layers.size()-1; layer_num > 0; --layer_num) {
        Layer &layer = m_layers[layer_num];
        Layer &prev_layer = m_layers[layer_num-1];

        for (int n = 0; n < layer.neuron_array.size() - 1; ++n) {
            layer.neuron_array[n].updateInputWeights(prev_layer);
        }
    }
    return m_error;
}

double Net::compute_loss(std::vector<double> &target_vals, string type) {
    Layer &output_layer = m_layers.back();
    double sample_error = 0.0;
    
    if (type == "huber") {
        for (int n=0; n < output_layer.neuron_array.size()-1; ++n) {
            double delta = target_vals[n] - output_layer.neuron_array[n].getOutputVal();
            sample_error += huber(delta);
        }
        sample_error /= (output_layer.neuron_array.size()-1);
    }

    else if (type == "rms") {
        for (int n=0; n < output_layer.neuron_array.size()-1; ++n) {
            double delta = target_vals[n] - output_layer.neuron_array[n].getOutputVal();
            sample_error += delta * delta;
        }
        sample_error /= (output_layer.neuron_array.size()-1);
        sample_error = sqrt(sample_error);
    }

    else {
        cout << "Unrecognized loss(): " << type << endl;
        assert(false);
    }

    return sample_error;
}


void Net::getResults(std::vector<double> &result_vals) {
    result_vals.clear();
    for (unsigned n = 0; n < m_layers.back().neuron_array.size() - 1; ++n) {
        result_vals.push_back(m_layers.back().neuron_array[n].getOutputVal());
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


void Neuron::feedForward(Layer &prev_layer, string act_type) {
    double sum = 0.0;

    // Input of this neuron = sum of prev_layer's outputs including bias node
    for (int n=0; n < prev_layer.neuron_array.size(); ++n)
        sum += prev_layer.neuron_array[n].getOutputVal() * prev_layer.neuron_array[n].m_output_weights[m_my_index].weight;
 
    m_output_val = Neuron::activation(sum, act_type);
}


void Neuron::calcOutputGradients(double target_val, string act_type) {
    double delta = target_val - m_output_val;
    m_gradient = delta * Neuron::activationDerivative(m_output_val, act_type);
}


void Neuron::calcHiddenGradients(Layer &next_layer, string act_type) {
    double dow = sumDOW(next_layer);
    m_gradient = dow * Neuron::activationDerivative(m_output_val, act_type);
}


void Neuron::updateInputWeights(Layer &prev_layer) {
    // The weights to be updated are in the Connection container
    // in the neurons in the preceding layer (i.e optimizer function)
    for (unsigned n = 0; n < prev_layer.neuron_array.size(); ++n) {
        Neuron &neuron = prev_layer.neuron_array[n];
        double old_delta_weight = neuron.m_output_weights[m_my_index].delta_weight;

        // Individual input, magnified by the gradient and learning rate
        // Also add momentum = a fraction of the previous delta weight
        double new_delta_weight = alpha * neuron.getOutputVal() * m_gradient + gamma * old_delta_weight;
                
        neuron.m_output_weights[m_my_index].delta_weight = new_delta_weight;
        neuron.m_output_weights[m_my_index].weight += new_delta_weight;
    }
}


double Neuron::sumDOW(const Layer &next_layer) const {
    double sum = 0.0;

    // Sum our contributions of the errors at the nodes we feed (Differential of Weights)
    for (unsigned n = 0; n < next_layer.neuron_array.size() - 1; ++n) {
        sum += m_output_weights[n].weight * next_layer.neuron_array[n].m_gradient;
    }

    return sum;
}


double Neuron::activation(double input, string type) {
    if (type == "tanh")         // output = [-1.0, 1.0]
        return tanh(input); 
    else if (type == "sigmoid") // output = [0.0, 1.0]
        return sigmoid(input);
    else if (type == "relu")
        return relu(input);
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
    else if (type == "relu")
        return relu_deriv(input);
    else {
        cout << "Unrecognized activation_deriv(): " << type << endl;
        assert(false);
    }
}

double sigmoid(double x) {
    double result = 1 / (1 + exp(-x));
    return result;
}

double huber(double x, double delta) {
    if ((int)(x*100) <= (int)(delta*100))
        return 0.5*x*x;
    else
        return delta*abs(x)-0.5*delta*delta;
}

double relu(double x) {
    if ((int)x >=0)
        return x;
    else
        return 0;
}

double relu_deriv(double x) {
    if ((int)x >=0)
        return 1;
    else
        return 0;
}