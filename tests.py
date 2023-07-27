import turtle
import json
import random
import math
import time
from turtle import forward, right, left

PAYLOAD = """"""

# generate tests for data stream from esp32
class Test:
    def __init__(self):
        self.toggle = 0
        self.payload = self.generate_hex_string(100)
        self.payload2 = self.generate_hex_string(100)
        self.itr = 0
    
    @staticmethod
    def generate_hex_string(length):
        hex_chars = '0123456789ABCDEF'
        hex_string = ''.join(random.choice(hex_chars) for _ in range(length))
        return hex_string

    # 1 tag moving around
    def generate_test_rand(self):
        if self.toggle:
            anchor = "42"
        else: anchor = "41"
        print(anchor)
        var1 = str(round(random.uniform(0.4, 0.7),2))
        payload = self.payload

        datastr = '{"links": {"A":"' + anchor + '", "R":"'+ var1 + '", "T": "428800CADE564557", "Payload":"'+str(payload)+'"}}'
        uwb_data = json.loads(datastr)
        uwb_list = uwb_data["links"]
        print(uwb_list)
        self.toggle = not self.toggle
        return uwb_list

    # 2 tags static
    def generate_test(self):
        var1 = 0.4
        var2 = 0.6
        if self.toggle == 0:
            data = '{"links": {"A":"41", "R":"0.4", "T":"428800CADE564557", "Payload":"' + str(self.payload) + '"}}'
            
        elif self.toggle == 1:
            data = '{"links": {"A":"42", "R":"0.6", "T":"428800CADE564557", "Payload":"' + str(self.payload) + '"}}'

        elif self.toggle == 2:
            data = '{"links": {"A":"42", "R":"0.6", "T":"418800CADE564557", "Payload":"' + str(self.payload2) + '"}}'

        elif self.toggle == 3:
            data = '{"links": {"A":"41", "R":"0.6", "T":"418800CADE564557", "Payload":"' + str(self.payload2) + '"}}'

        uwb_data = json.loads(data)
        uwb_list = uwb_data["links"]
        print(uwb_list)
        self.toggle = self.toggle + 1
        if self.toggle > 3: self.toggle = 0
        return uwb_list
    
    def generate_test_rand_two(self):
        tagid = str(random.randint(41,42)) + "8800CADE564557"
        range = str(round(random.uniform(0.4, 0.7),2))
        if self.toggle == 0:
            anchor = "42"
            
        else:
            anchor = "41"
            
        print(anchor)
        

        datastr = '{"links": {"A":"' + anchor + '", "R":"'+ range + '", "T": "'+tagid+'"}}'

        uwb_data = json.loads(datastr)
        uwb_list = uwb_data["links"]
        print(uwb_list)
        self.toggle = self.toggle + 1
        if self.toggle > 3: self.toggle = 0
        return uwb_list
    
    def generate_test_approaching_one(self):
        if self.toggle:
            anchor = "42"
        else: anchor = "41"
        print(anchor)
        var1 = str(round(random.uniform(0.4, 0.7),2))
        payload = self.payload
        a_range = [1, 0.8, 0.6, 0.5, 0.45, 0.43, 0.41, 0.4]

        datastr = '{"links": {"A":"' + anchor + '", "R":"'+ str(a_range[self.itr]) + '", "T": "428800CADE564557", "Payload":"'+str(payload)+'"}}'

        uwb_data = json.loads(datastr)
        uwb_list = uwb_data["links"]
        print(uwb_list)

        self.toggle = not self.toggle
        if not self.toggle:
            self.itr += 1
            if self.itr >= len(a_range):
                self.itr = 0
        
        return uwb_list