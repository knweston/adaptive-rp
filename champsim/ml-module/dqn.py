import random
from collections import deque

import warnings
with warnings.catch_warnings():
    warnings.filterwarnings("ignore",category=FutureWarning)
    import numpy as np
    from numpy import savetxt
    from numpy import loadtxt
    from tensorflow.keras import models
    from tensorflow.keras import layers
    from tensorflow.keras import optimizers
    from tensorflow.keras.optimizers import Adam
    from tensorflow.keras.losses import Huber as huber_loss
    from tensorflow.keras.initializers import GlorotNormal as GlorotNormal

import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

try:
    from tensorflow.python.util import module_wrapper as deprecation
except ImportError:
    from tensorflow.python.util import deprecation_wrapper as deprecation
deprecation._PER_MODULE_WARNING_LIMIT = 0

# ==================================================================================== #
# CLASS DQN_ENGINE
# ==================================================================================== #
class DQN:
    def __init__(self, _cache_name, _num_features, _num_output, _app_name, _batch, _checkpt="none"):
        self.cache_name   = _cache_name
        self.gamma        = 0.90    # discount factor
        self.alpha        = 0.0005  # learning rate
        self.eps          = 1.00    # exploration rate (initially 1.0)
        self.decay        = 1.00    # decay rate of eps
        self.STOP_EPS     = 0.05    # minimum eps
        self.UPDATE_INTVL = 10
        self.num_actions  = _num_output
        self.num_features = _num_features
        self.app_name     = _app_name
        self.batch_no     = _batch
            
        self.network        = self.createModel(self.num_features, self.num_actions, _checkpt)
        self.target_network = self.createModel(self.num_features, self.num_actions, _checkpt)
        self.best_model     = self.createModel(self.num_features, self.num_actions, _checkpt)

        self.replay_buffer = deque(maxlen=64)
        print("New model created\n")

        # statistics
        self.num_retrains = 0
        self.action_count = [0] * self.num_actions
        self.total_reward = 0.0

    def createModel(self, _num_input, _num_output, _checkpoint):
        model = models.Sequential()
        model.add(layers.Dense(512, activation='relu', input_shape=(_num_input,), kernel_initializer=GlorotNormal(), kernel_regularizer='l2'))
        model.add(layers.Dense(_num_output, activation='linear'))
        if _checkpoint != "none":
            model.load_weights(_checkpoint)
        model.compile(optimizer=Adam(learning_rate=self.alpha), loss=huber_loss())
        return model
    
    def predict(self, state):
        action = 0
        if np.random.randint(0,99) < int(self.eps*100):
            action = np.random.randint(0,self.num_actions)
        else:
            x_input = [state]
            x_input = np.array(x_input)
            action = np.argmax(self.network.predict(x_input)) 

        # collect statistics
        self.action_count[action] += 1
        return action

    def add_sample(self, data):
        state      = []
        next_state = []
        action     = data[-2]
        reward     = data[-1]
        
        state_length = self.num_features
        for index in range(state_length):
            state.append(data[index])
        for index in range(state_length, 2*state_length):
            next_state.append(data[index])

        self.replay_buffer.append((state, int(action), next_state, reward))
        self.total_reward += reward


    def get_training_parameters(self):
        # if len(self.replay_buffer) <= 32:
        #     return len(self.replay_buffer), len(self.replay_buffer), 1
        # elif len(self.replay_buffer) > 32 and len(self.replay_buffer) <= 256:
        #     return 32, 16, 2
        # elif len(self.replay_buffer) > 256 and len(self.replay_buffer) <= 1024:
        #     return 64, 16, 2
        # elif len(self.replay_buffer) > 1024 and len(self.replay_buffer) <= 2048:
        #     return 128, 32, 2
        # else:
        #     return 256, 32, 3

        if len(self.replay_buffer) <= 16:
            return len(self.replay_buffer), len(self.replay_buffer), 1
        else:
            return len(self.replay_buffer), 16, 1


    def retrain(self):
        # sanity check: there must be something in the replay buffer
        assert(len(self.replay_buffer))

        draw_size, train_batch_size, num_epochs = self.get_training_parameters()
        batch = random.sample(self.replay_buffer, draw_size)

        states_batch, actions_batch, next_states_batch, rewards_batch = map(np.array, zip(*batch))
        current_Q_batches = self.network.predict(states_batch)
        next_Q_batches = self.target_network.predict(next_states_batch)

        # update new Q-value: Q(s,a) = Q(s,a) + alpha * (reward + gamma * maxQ(s',a') - Q(s,a))
        for index in range(len(current_Q_batches)):
            current_Q_batches[index][actions_batch[index]] += self.alpha * \
                                                              (rewards_batch[index] + \
                                                              self.gamma * np.max(next_Q_batches[index]) - \
                                                              current_Q_batches[index][actions_batch[index]])  
        
        self.network.fit(states_batch, current_Q_batches, batch_size=train_batch_size, \
                        verbose=False, epochs=num_epochs)
        self.num_retrains += 1

        if self.num_retrains % self.UPDATE_INTVL == 0:
            self.target_network.set_weights(self.network.get_weights())
        
        
    def save_model(self):
        weight_dir  = os.path.dirname(os.getcwd())+'/data/weights/'+str(self.num_features)
        weight_file = weight_dir+'/'+self.app_name+'_b'+self.batch_no
        if not os.path.exists(weight_dir):
            os.makedirs(weight_dir)
        self.target_network.save(weight_file)
    
    def finalize_episode(self):
        self.eps   *= self.decay        
        if int(self.eps*100) <= int(self.STOP_EPS*100):
            self.eps = 0
        self.total_reward = 0.0 
        self.save_model()       
        
        