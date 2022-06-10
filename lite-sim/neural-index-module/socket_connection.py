import os
import socket

# ==================================================================================== #
# CLASS PREDICTION_SERVER
# ==================================================================================== #
class SocketConnection:
    def __init__(self, pred_engine):
        self.prediction_engine  = pred_engine
        self.done_episodes      = 0
        self.num_episodes       = 0
        self.complete_signal    = 0
        self.buffer_size        = 16*1024*1024
        self.MAX_SAMPLE_PACKETS = 6
        self.MAX_STATE_PACKETS  = 2

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
    
    # process client request
    def process_request(self, request, connection):
        
        if len(request) == 0:
            return
        
        elif request == "make prediction":
            reply = "Request received: make inference"
            connection.send(reply.encode())
            state = []
            for packet in range(self.MAX_STATE_PACKETS):
                new_chunk = []
                data = connection.recv(self.buffer_size).decode()
                if len(data) == 0:
                    reply = "Empty message"
                else:
                    new_chunk = data.split("_")
                    new_chunk = [float(numeric_str) for numeric_str in new_chunk]
                    state.extend(new_chunk)
                    if packet < (self.MAX_STATE_PACKETS-1):
                        reply = "Data received"
                        connection.send(reply.encode())

            prediction = self.prediction_engine.predict(state)
            reply = str(prediction)
            connection.send(reply.encode())

        elif request == "new sample":
            reply = "Request received: add sample"
            connection.send(reply.encode())
            sample_data = []
            for packet in range(self.MAX_SAMPLE_PACKETS):
                new_chunk = []
                data = connection.recv(self.buffer_size).decode()
                if len(data) == 0:
                    reply = "Empty message"
                else:
                    new_chunk = data.split("_")
                    new_chunk = [float(numeric_str) for numeric_str in new_chunk]
                    sample_data.extend(new_chunk)
                    if packet < (self.MAX_SAMPLE_PACKETS-1):
                        reply = "Data received"
                        connection.send(reply.encode())

            self.prediction_engine.addSample(sample_data)
            reply = "New sample added successfully"
            connection.send(reply.encode())

        elif request == "retrain":
            if self.prediction_engine.eps > 0:
                self.prediction_engine.retrain()
            reply = str(self.prediction_engine.num_retrains)
            connection.send(reply.encode())               

        elif request == "disconnect":
            self.num_episodes += 1
            self.complete_signal = 1

            reply = str(self.prediction_engine.eps) + "_" + str(self.prediction_engine.total_reward)
            connection.send(reply.encode())
            num_hits = int(connection.recv(self.buffer_size).decode())
            self.prediction_engine.done_episode_update(num_hits)
            reply = "Server disconnected successfully"
            connection.send(reply.encode())
            
        else:
            reply = "unknown request"
            connection.send(reply.encode())

    def start(self, port_no, ipaddress):
        max_episodes = 250

        sock = self.start_connection(port_no, ipaddress)
        while self.num_episodes < max_episodes:
            # Establish connection with client 0.
            conn, addr = sock.accept()
            print('Connected to ', addr)
            
            self.complete_signal = 0
            while self.complete_signal != 1:
                request = conn.recv(self.buffer_size).decode()
                self.process_request(request, conn)

        # Close the connection with the client
        conn.close()
    