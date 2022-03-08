from encoder import Encoder

class LRU:
    def __init__(self, _sets, _ways):
        self.num_set = _sets
        self.num_way = _ways        
        self.lru_position = [[j for j in range(self.num_way)] for i in range(self.num_set)]
    
    def get_victim(self, addr, ip, setid, access_type, victim_set_array):
        for way in range(self.num_way):
            if self.lru_position[setid][way] == (self.num_way - 1):
                return way
    
    def update(self, addr, setid, wayid, ip, is_hit):
        hit_position = self.lru_position[setid][wayid]
        for way in range(self.num_way):
            if self.lru_position[setid][way] < hit_position:
                self.lru_position[setid][way] += 1
        self.lru_position[setid][wayid] = 0


class ZPL():
    def __init__(self, _sets, _ways):
        self.base_rp = LRU(_sets, _ways)
        self.num_set = _sets
        self.num_way = _ways
        self.hit_table = [[0 for j in range(self.num_way)] for i in range(self.num_set)]
        self.ip_table  = [[0 for j in range(self.num_way)] for i in range(self.num_set)]
        self.model = None


    def initialize_model(self, input_dim, comp_dim, app, checkpt="None"):
        self.model = Encoder(input_dim, comp_dim, app, checkpt)


    def get_victim(self, addr, ip, setid, access_type, victim_set_array):
        # fill invalid lines first
        for wayid, data in enumerate(victim_set_array):
            if not data:
                return wayid
        
        bypass = False
        if access_type != 3: # if not WRITEBACK, check bypass
            bypass = self.model.predict(addr, ip)
        if bypass:
            return self.num_way
        else:
            # get LRU victim
            victim = self.base_rp.get_victim(addr, ip, setid, access_type, victim_set_array)

            # create new sample if the victim has never been re-referenced
            if not self.hit_table[setid][victim] and self.ip_table[setid][victim]:
                self.model.add_sample(victim_set_array[victim], self.ip_table[setid][victim])
            return victim 
        

    def update(self, addr, setid, wayid, ip, is_hit):
        self.base_rp.update(addr, setid, wayid, ip, is_hit)
        if not is_hit:
            self.hit_table[setid][wayid] = 0
            self.ip_table[setid][wayid]  = ip
        else:
            self.hit_table[setid][wayid] = 1