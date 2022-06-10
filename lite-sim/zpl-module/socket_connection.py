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

        elif request.find("make prediction") != -1:
            # reply = "Request received: make inference"
            # connection.send(reply.encode())
            # data = connection.recv(self.buffer_size).decode()
            
            sample = request.split("_")
            sample.pop(0)
            sample = [int(numeric_str) for numeric_str in sample]
            prediction = self.prediction_engine.predict(sample)
            reply = str(prediction)
            connection.send(reply.encode())

        elif request.find("new sample") != -1:
            # reply = "Request received: add sample"
            # connection.send(reply.encode())
            # data = connection.recv(self.buffer_size).decode()
            # new_batch = data.split("_")
            # new_batch = [int(numeric_str) for numeric_str in new_batch]

            data = request.split("_")
            data.pop(0)
            data = [int(numeric_str) for numeric_str in data]

            self.prediction_engine.add_sample(data)
            reply = "New sample added successfully"
            connection.send(reply.encode())             

        elif request == "disconnect":
            self.num_episodes += 1
            self.complete_signal = 1
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
    