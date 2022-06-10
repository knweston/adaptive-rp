import sys
import argparse

def parse_mpki(_filename):
    infile = open(_filename, "r")
    total_miss = 0
    write_miss = 0
    for line in infile:
        if line.find("LLC TOTAL") != -1:
            tokens = line.split(" ")
            tokens = remove_spaces(tokens)
            total_miss = int(tokens[-1])
            
        elif line.find("LLC WRITEBACK") != -1:
            tokens = line.split(" ")
            tokens = remove_spaces(tokens)
            write_miss = int(tokens[-1])
            
    if total_miss > 0 or write_miss > 0:
        return (total_miss-write_miss)/1000000

    # we should not reach here
    print("LLC MPKI STATS NOT FOUND!")
    exit(1)


def parse_miss_rate(_filename):
    total_accesses = 0
    infile = open(_filename, "r")
    for line in infile:
        if line.find("LLC TOTAL") != -1:
            tokens = line.split(" ")
            tokens = remove_spaces(tokens)
            miss = float(tokens[-1])
            total_accesses = float(tokens[3])
            return miss/total_accesses

    # we should not reach here
    print("LLC MISS RATE STATS NOT FOUND!")
    exit(1)


def parse_ipc(_filename):
    infile = open(_filename, "r")
    for line in infile:
        if line.find("CPU 0 cumulative IPC:") != -1:
            tokens = line.split(" ")
            tokens = remove_spaces(tokens)
            ipc = float(tokens[4])
            return ipc
    
    # we should not reach here
    print("LLC IPC STATS NOT FOUND!")
    exit(1)


def remove_spaces(lst):
    # lst.remove('\n')
    while "" in lst:
        lst.remove("")
    return lst

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--filename", required=True, type=str)
    parser.add_argument("-s", "--statistics", required=True, type=str)
    args = parser.parse_args()
    infile = open(args.filename, "r")
    
    for line in infile:
        demanded_stat = None
        if args.statistics == "ipc":
            demanded_stat = parse_ipc(line.strip())
        elif args.statistics == "mpki":
            demanded_stat = parse_mpki(line.strip())
        elif args.statistics == "miss-rate":
            demanded_stat = parse_miss_rate(line.strip())
        
        print(demanded_stat)

if __name__ == "__main__":
    main()