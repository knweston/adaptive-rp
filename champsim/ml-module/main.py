import sys
from dqn import DQN 
from socket_connection import SocketConnection
import argparse

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
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--app_name", required = True, type=str)
    parser.add_argument("-c", "--config", required = True, type=str)
    parser.add_argument("-l", "--levels", required = True, type=str)
    
    parser.add_argument("-w", "--weights", required = False, type=str)
    parser.add_argument("-f", "--num_features", required = True, type=str)
    parser.add_argument("-o", "--num_outputs", required = True, type=str)
    args = parser.parse_args()

    port       = readPortConfig(args.config)
    ip_address = readIPConfig(args.config)

    cache_levels = args.levels.split("_")
    num_features = [int(features) for features in args.num_features.split("_")]
    num_outputs  = [int(outputs) for outputs in args.num_outputs.split("_")]
    if args.weights == "none":
        weight_files = ["none"] * len(cache_levels)
    else:
        weight_files = args.weights.split("_")

    pred_engines = []

    for idx in range(len(cache_levels)):
        pred_engines.append( DQN(cache_levels[idx], num_features[idx], 
                             num_outputs[idx], args.app_name,           
                             args.config.split('/')[-1][:-4], _checkpt=weight_files[idx]))

    server = SocketConnection(pred_engines, cache_levels, num_features)
    server.start(port, ip_address)
    
    
if __name__ == "__main__":
    main()