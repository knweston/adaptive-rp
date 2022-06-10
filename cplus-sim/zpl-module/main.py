import sys
from encoder import Encoder 
from socket_connection import SocketConnection

# LLC: 256KB direct-mapped

# ==================================================================================== #
# MAIN FUNCTION
# ==================================================================================== #
def readPortConfig(filename):
    port_num = 0
    conf_file = open(filename, "r")
    while True:
        line = conf_file.readline()
        if not line:
            break
        else:
            if line.find("port") != -1:
                port_num = int(line.split("=")[1])
                break
    return port_num

def readIPConfig(filename):
    conf_file = open(filename, "r")
    while True:
        line = conf_file.readline()
        if not line:
            break
        else:
            if line.find("ipaddress") != -1:
                return (line.split("=")[1]).rstrip().lstrip()

def main():
    if len(sys.argv) != 6:
        print("Usage: " + sys.argv[0] + " [APP_NAME] [WEIGHT_FILE] [CONFIG_FILE] [INPUT_DIM] [COMPRESS_DIM]")
        exit(1)
    app_name     = sys.argv[1]
    weight_file  = sys.argv[2]
    server_conf  = sys.argv[3]  
    input_dim    = int(sys.argv[4])
    compress_dim = int(sys.argv[5])
    port         = readPortConfig(server_conf)
    ip_address   = readIPConfig(server_conf)

    # Currently num_features = 8 per cache line
    pred_engine = Encoder(input_dim, compress_dim, app_name, _checkpt=weight_file)
    server = SocketConnection(pred_engine)
    server.start(port, ip_address)
    
    
if __name__ == "__main__":
    main()