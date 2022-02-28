import os
import socket

# ==================================================================================== #
# CLASS PREDICTION_SERVER
# ==================================================================================== #
class SocketConnection:
    def __init__(self, pred_engines, cache_levels, num_features):
        self.models             = pred_engines
        self.cache_names        = cache_levels
        self.num_episodes       = 0
        self.complete_signal    = [0] * len(pred_engines)
        self.buffer_size        = 16*1024*1024

        self.CHUNK_SIZE = [0] * len(pred_engines)
        self.NUM_CHUNKS = [0] * len(pred_engines)

        for idx, cache in enumerate(self.cache_names):
            if cache == "L1D":
                self.CHUNK_SIZE[idx] = num_features[idx]
                self.NUM_CHUNKS[idx] = 1

            elif cache == "L2C":
                if (num_features[idx] > 1024):
                    self.CHUNK_SIZE[idx] = 1024
                    self.NUM_CHUNKS[idx] = int(num_features[idx] / self.CHUNK_SIZE[idx])
                else:
                    self.CHUNK_SIZE[idx] = num_features[idx]
                    self.NUM_CHUNKS[idx] = 1

            elif cache == "LLC":
                self.CHUNK_SIZE[idx] = 1024
                self.NUM_CHUNKS[idx] = int(num_features[idx] / self.CHUNK_SIZE[idx])

        self.MAX_STATE_PACKETS  = self.NUM_CHUNKS
        self.MAX_SAMPLE_PACKETS = [int(state_packets * 2 + 2) for state_packets in self.MAX_STATE_PACKETS]

        print("cache levels:   ", self.cache_names)
        print("chunk size:     ", self.CHUNK_SIZE)
        print("state packets:  ", self.MAX_STATE_PACKETS)
        print("sample packets: ", self.MAX_SAMPLE_PACKETS)

    # start connection with client
    def start_connection(self, port_no, ipaddress=''):
        # create a socket object
        s = socket.socket()
        print("Socket successfully created")

        # max wait time = 1200s
        s.settimeout(1200)

        # reserve a port on your computer in our
        # case it is 12345 but it can be anything
        port = port_no

        # next bind to the port
        # we have not typed any ip in the ip field
        # instead we have input an empty string
        # this makes the server listen to requests
        # coming from other computers on the network
        s.bind((ipaddress, port))
        print("socket binded to %s" % (port))

        # put the socket into listening mode
        s.listen(5)
        print("socket is listening")
        return s
    

    def find_model_by_name(self, name):
        for idx in range(len(self.models)):
            if self.models[idx].cache_name == name:
                return idx
        return -1

    # process client request
    def process_request(self, request, connection):
        
        if len(request) == 0:
            return

        
        index = -1
        if request.find("L1D") != -1:
            index = self.find_model_by_name("L1D")
        elif request.find("L2C") != -1:
            index = self.find_model_by_name("L2C")
        elif request.find("LLC") != -1:
            index = self.find_model_by_name("LLC")
        
        if index < 0:
            print("Model not found")
            exit(1)

        if request.find("make prediction") != -1:
            reply = "Request received: make inference " + self.models[index].cache_name
            print(reply)
            connection.send(reply.encode())
            state = []
            for packet in range(self.MAX_STATE_PACKETS[index]):
                new_chunk = []
                data = connection.recv(self.buffer_size).decode()
                if len(data) == 0:
                    reply = "Empty message"
                else:
                    new_chunk = data.split("_")
                    new_chunk = [float(numeric_str) for numeric_str in new_chunk]
                    state.extend(new_chunk)

                    if self.models[index].cache_name == "L2C":
                        print("data received: %d" % len(state))

                    if packet < (self.MAX_STATE_PACKETS[index]-1):
                        reply = "Data received"
                        connection.send(reply.encode())

            prediction = self.models[index].predict(state)
            reply = str(prediction)
            connection.send(reply.encode())


        elif request.find("new sample") != -1:
            reply = "Request received: add sample " + self.models[index].cache_name
            connection.send(reply.encode())
            sample_data = []
            for packet in range(self.MAX_SAMPLE_PACKETS[index]):
                new_chunk = []
                data = connection.recv(self.buffer_size).decode()
                if len(data) == 0:
                    reply = "Empty message"
                else:
                    new_chunk = data.split("_")
                    new_chunk = [float(numeric_str) for numeric_str in new_chunk]
                    sample_data.extend(new_chunk)
                    if packet < (self.MAX_SAMPLE_PACKETS[index]-1):
                        reply = "Data received"
                        connection.send(reply.encode())

            self.models[index].addSample(sample_data)
            reply = "New sample added successfully"
            connection.send(reply.encode())


        elif request.find("retrain") != -1:
            if self.models[index].eps > 0:
                self.models[index].retrain()
            reply = str(self.models[index].num_retrains)
            connection.send(reply.encode())               


        elif request.find("disconnect") != -1:
            self.complete_signal[index] = 1

            reply = str(self.models[index].eps) + "_" + str(self.models[index].total_reward)
            connection.send(reply.encode())
            self.models[index].done_episode_update()

        else:
            reply = "unknown request"
            connection.send(reply.encode())


    def start(self, port_no, ipaddress):
        max_episodes = 200

        sock = self.start_connection(port_no, ipaddress)
        while self.num_episodes < max_episodes:
            # Establish connection with client 0.
            conn, addr = sock.accept()
            print('Connected to ', addr)
            
            self.complete_signal = [0] * len(self.models)
            while 0 in self.complete_signal:
                request = conn.recv(self.buffer_size).decode()
                self.process_request(request, conn)
                
            self.num_episodes += 1

        # Close the connection with the client
        conn.close()
    