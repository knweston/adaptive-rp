import warnings
with warnings.catch_warnings():
    warnings.filterwarnings("ignore",category=FutureWarning)
    from tensorflow.keras import models, layers, Input, Model
    from tensorflow.keras import layers, regularizers
    import numpy as np
    from tensorflow.keras.losses import Huber as huber_loss
from itertools import permutations

from random import shuffle
from collections import deque
from copy import deepcopy

import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

try:
    from tensorflow.python.util import module_wrapper as deprecation
except ImportError:
    from tensorflow.python.util import deprecation_wrapper as deprecation
deprecation._PER_MODULE_WARNING_LIMIT = 0


class Encoder:
    def __init__(self, _org_dim, _comp_dim, _app_name, _checkpt="None"):
        self.dim = _comp_dim
        self.autoencoder, self.encoder, self.decoder = self.create_models(_org_dim, _comp_dim)
        self.autoencoder.compile(optimizer='adam', loss=huber_loss())
        self.sample_buffer = []
        self.threshold = 0
        
        self.num_new_samples = 0
        self.num_trains = 0
        
        # predefined parameter
        self.BUFFER_SIZE = 256
        self.VAL_CUTOFF  = (-1)*int(self.BUFFER_SIZE/8)
        self.ADDR_LENGTH = 16


    def create_models(self, _num_features, _hidden_dim):
        # create an autoencoder
        input_layer  = Input(shape=(_num_features,))
        hidden_layer = layers.Dense(_hidden_dim, activation='sigmoid', activity_regularizer=regularizers.l1(10e-5))(input_layer)
        output_layer = layers.Dense(_num_features, activation='sigmoid')(hidden_layer)

        autoencoder =  Model(input_layer, output_layer)

        # create the encoder
        encoder = Model(input_layer, hidden_layer)

        # create the decoder
        decoder_input  = Input(shape=(_hidden_dim,))
        decoder_output = autoencoder.layers[-1](decoder_input)
        decoder = Model(decoder_input, decoder_output)

        return autoencoder, encoder, decoder


    def train(self, dataset):
        output = self.autoencoder.predict(dataset[self.VAL_CUTOFF:])
        sampled_loss = huber_loss()(dataset[self.VAL_CUTOFF:], output)
        
        if int(sampled_loss*10000) > 0:
            x_train = np.array(dataset)
            x_test  = np.array(dataset[self.VAL_CUTOFF:])
            self.autoencoder.fit(x_train, x_train, epochs=25, batch_size=8, shuffle=True, verbose=True)
            output = self.autoencoder.predict(x_test)
            self.threshold = huber_loss()(x_test, output)
            self.num_trains += 1
            print("sampled_loss: %.6f. new_threshold: %.6f. num_trains: %d" % (sampled_loss, self.threshold, self.num_trains))
            

    def add_sample(self, data):
        num_pairs = int(len(data)/2)
        for i in range(num_pairs):
            new_sample = self.int_to_binary(data[i*2])
            new_sample.extend(self.int_to_binary(data[i*2+1]))
            self.sample_buffer.append(new_sample)
        self.num_new_samples += num_pairs
        
        # remove old samples
        while len(self.sample_buffer) > self.BUFFER_SIZE:
            self.sample_buffer.pop(0)

        # retrain model if sample buffer is full 
        if (self.num_new_samples >= int(self.BUFFER_SIZE*3/4)) and (len(self.sample_buffer) == self.BUFFER_SIZE):
            self.train(deepcopy(self.sample_buffer))
            self.num_new_samples = 0
        print("total samples: %d. new samples: %d" % (len(self.sample_buffer), self.num_new_samples))
        

    def predict(self, sample):
        x_input = self.int_to_binary(sample[0])
        x_input.extend(self.int_to_binary(sample[1]))
        x_input = np.array([x_input])

        output = self.autoencoder.predict(x_input)
        loss = huber_loss()(x_input, output)
        
        if int(loss*100000) > int(self.threshold*100000):
            return 0
        else:
            print("bypassed sample: ", end="")
            print(sample, end="")
            print(". threshold: %d. loss: %d" % (int(self.threshold*100000), int(loss*100000)))
            return 1


    def int_to_binary(self, integer):
        string = format(integer, "b")
        binary_list = []
        for c in string:
            binary_list.append(int(c))
        
        # zero padding to make 32-bit address
        num_padding = self.ADDR_LENGTH - len(binary_list)
        if num_padding > 0:
            for i in range(num_padding):
                binary_list.insert(0, 0)
        elif num_padding < 0:
            for i in range((-1)*num_padding):
                binary_list.pop(0)
        return binary_list

    def save_models(self):
        self.autoencoder.save("autoencoder.h5")
        self.encoder.save("encoder.h5")
        self.decoder.save("decoder.h5")

    def load_models(self):
        self.autoencoder.load_weights("autoencoder.h5")
        self.encoder.load_weights("encoder.h5")
        self.decoder.load_weights("decoder.h5")