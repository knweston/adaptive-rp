import os
import sys
import argparse
import time
from math import log2
from replacement import LRU, ZPL

LLC_SET   = 2048
LLC_WAY   = 16

LOAD      = 0
RFO       = 1
PREFETCH  = 2
WRITEBACK = 3
NUM_TYPES = 4   # LOAD/RFO/PREFETCH/WRITEBACK
PROGRESS_INTERVAL = 1000

ACCESS   = [0] * NUM_TYPES
HIT      = [0] * NUM_TYPES
MISS     = [0] * NUM_TYPES
EVICTION = [0] * LLC_SET
BYPASS   = 0

class Access:
    def __init__(self, _addr=0, _ip=0, _type=0):
        self.addr = _addr
        self.ip   = _ip
        self.type = _type


class CACHE:
    def __init__(self, _sets, _ways):
        self.num_set = _sets
        self.num_way = _ways
        self.blocks_array = [[0 for j in range(self.num_way)] for i in range(self.num_set)]
        self.rp = ZPL(_sets, _ways)
        # self.rp = LRU(_sets, _ways)

    def get_set(self, addr):
        return addr & ((1 << int(log2(self.num_set))) - 1)

    def check_hit(self, addr, setid):
        for wayid, data in enumerate(self.blocks_array[setid]):
            if data == addr:
                return wayid 
        return -1
    
    def fill_cache(self, addr, setid, victim_way):
        self.blocks_array[setid][victim_way] = addr

    def update_rp(self, addr, setid, wayid, ip, is_hit):
        self.rp.update(addr, setid, wayid, ip, is_hit)

    def get_victim(self, addr, ip, access_type, setid):
        return self.rp.get_victim(addr, ip, setid, access_type, self.blocks_array[setid])

    def print_cache(self):
        for set_arr in self.blocks_array:
            print(set_arr)


def print_stats(trace_file):
    print("========================================================================")
    print("TRACE   = " + trace_file)
    print("LLC_SET = %d" % LLC_SET)
    print("LLC_WAY = %d" % LLC_WAY)
    print("========================================================================")
    print("LLC FINAL STATISTICS")
    print("========================================================================")
    print("ACCESS   = %d" % sum(ACCESS))
    print("HIT      = %d" % sum(HIT))
    print("MISS     = %d. RATE: %.3f%%" % (sum(MISS), 100*sum(MISS)/sum(ACCESS)))
    print("EVICTION = %d" % sum(EVICTION))
    print("BYPASS   = %d" % BYPASS)
    print("========================================================================")


def read_trace(trace_file):
    trace = []
    infile = open(trace_file, "r")
    for line in infile:
        tokens = line.split(" ")
        new_access = Access(int(tokens[0]), int(tokens[1]))
        if line.find("LOAD") != -1:
            new_access.type = 0
        elif line.find("RFO") != -1:
            new_access.type = 1
        elif line.find("PREFETCH") != -1:
            new_access.type = 2
        elif line.find("WRITEBACK") != -1:
            new_access.type = 3
        else:
            print("Invalid Access Type: " + tokens[2])
            print(line)
            exit(1)
        trace.append(new_access)
    return trace


def main_loop(TRACE, LLC):
    global BYPASS

    start_time = time.clock()
    for c_ptr, access in enumerate(TRACE):

        if c_ptr % PROGRESS_INTERVAL == 0:
            end_time = time.clock()
            print("\n===== Processed Accesses: %d. Bypasses: %d. Time: %d seconds =====" % (c_ptr, BYPASS, end_time-start_time))
            start_time = end_time

        setid = LLC.get_set(access.addr)
        hit_way = LLC.check_hit(access.addr, setid)       
        
        if hit_way >= 0: # LLC hit

            # update the replacement policy with cache hit signal
            if access.type != WRITEBACK:
                LLC.update_rp(access.addr, setid, hit_way, access.ip, 1)
            
            # update statistics
            HIT[access.type]    += 1
            ACCESS[access.type] += 1
        
        else: # LLC miss

            # get victim way
            victim_way = LLC.get_victim(access.addr, access.ip, access.type, setid)

            if victim_way == LLC_WAY: # if bypass
                # update statistics
                MISS[access.type]   += 1
                ACCESS[access.type] += 1
                BYPASS              += 1

            else:
                # update statistics
                MISS[access.type]   += 1
                ACCESS[access.type] += 1
                if LLC.blocks_array[setid][victim_way]:
                    EVICTION[setid] += 1

                # fill cache with new data
                LLC.fill_cache(access.addr, setid, victim_way)

                # update the replacement policy with miss signal
                LLC.update_rp(access.addr, setid, victim_way, access.ip, 0)


def main():    
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--app", required=True, type=str)
    parser.add_argument("-t", "--trace", required=True, type=str)
    parser.add_argument("-w", "--weight", required=False, type=str)
    parser.add_argument("-i", "--num_input", required=False, type=int)
    parser.add_argument("-c", "--compress_output", required=False, type=int)
    args = parser.parse_args()
    
    # create cache
    cache = CACHE(LLC_SET, LLC_WAY)
    print("========================================================================")
    print("Cache Initialization Completed")

    # create prediction engine
    if (args.num_input) and (args.compress_output):
        cache.rp.initialize_model(args.num_input, args.compress_output, args.app, args.weight)
        print("Encoder Initialization Completed")
        
    # read trace
    trace = read_trace(args.trace)
    print("Trace Initialization Completed")
    print("========================================================================")
    # run main loop
    main_loop(trace, cache)

    # print final statistics
    print_stats(args.trace)

if __name__ == "__main__":
    main()