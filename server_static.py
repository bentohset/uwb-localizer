import time
import cmath
import socket
import random
import json
import pygame
import math

TESTING = False

######Calibration/Structure Setup########
anchor1_pos = (200, 400)
anchor2_pos = (400, 400)
anchor1_id = "41"
anchor2_id = "42"
anchor_dist = 0.8
anchor_pos_list = [anchor1_pos, anchor2_pos]
anchor_name_list = ["anchor1", "anchor2"]
pixel_scale = abs(anchor1_pos[0] - anchor2_pos[0]) / anchor_dist

######Pygame Display Settings####
white = (255, 255, 255)
red = (255, 0, 0)
green = (0, 255, 0)
blue = (0, 0, 255)
grey = (110,110,110)
black = (0,0,0)
yellow= (255, 255, 0)
need_render = True
window_width = 600
window_height = 800


def init_udp():
    hostname = socket.gethostname()
    UDP_IP = socket.gethostbyname(hostname)
    print("***Local ip:" + str(UDP_IP) + "***")
    UDP_PORT = 80
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((UDP_IP, UDP_PORT))
    print("Socket created")
    sock.listen(1) 
    print("Waiting for a connection...")
    data, addr = sock.accept()
    print("Device connected")

    return data, addr

def generate_test_rand():
    var1 = str(round(random.uniform(0.4, 0.7),2))
    var2 = str(round(random.uniform(0.4, 0.7),2))
    # data = {
    #     "links": [{"A":"81", "R":str(var1)}, {"A":"82","R":str(var2)}]
    # }
    datastr = '{"links": [{"A":"81", "R":"'+ var1 +'"}, {"A":"82","R":"'+ var2+'"}]}'

    return datastr

def generate_test():
    var1 = 0.4
    var2 = 0.6
    data = """{
        "links": [{"A":"81", "R":"0.4"}, {"A":"82","R":"0.5"}]
    }"""

    return data

def init_game():
    pygame.init()
    window = pygame.display.set_mode((window_width, window_height))
    clock = pygame.time.Clock()
    return window, clock

def read_data(data):
    if TESTING:
        line =  data
    else: line = data.recv(1024).decode('UTF-8')
    uwb_list = []
    try: 
        uwb_data = json.loads(line)
        print(uwb_data)
        uwb_list = uwb_data["links"]
        for uwb_anchor in uwb_list:
            print(uwb_anchor)
    except:
        print("error")
    print("")

    return uwb_list

def calc_tag_pos(a1, a2):
    res = 0,0
    try:
        cos_a = (a1 * a1 + anchor_dist*anchor_dist - a2 * a2) / (2 * a1 * anchor_dist)
        x = a1 * cos_a
        y = a1 * math.sqrt(1 - cos_a * cos_a)
        res = round(x.real,2), round(y.real,2)

    except:
        res = 0,0

    return res

def scale_to_pixel(pos):
    return (pos[0] * pixel_scale, pos[1] * pixel_scale)

def render_data(screen, tag_pos):
    screen.fill((0, 0, 0))
    tag_pos_px = scale_to_pixel(tag_pos)
    # Render the positions of the anchors and tag
    pygame.draw.circle(screen, (255, 0, 0), anchor1_pos, 10)  # Red anchor
    pygame.draw.circle(screen, (0, 255, 0), anchor2_pos, 10)  # Green anchor
    pygame.draw.circle(screen, (0, 0, 255), tag_pos_px, 10)  # Blue tag
    pygame.display.flip()
    
def main():
    if not TESTING:
        data, addr = init_udp()
    screen, clock = init_game()
    a1_range = 0.00
    a2_range = 0.00
    running = True

    while running:
        if TESTING:
            data = generate_test_rand()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                pygame.quit()

        node_count = 0
        list = read_data(data)
        for anchor in list:
            if anchor["A"] == anchor1_id:
                a1_range = float(anchor["R"])
                node_count += 1
            if anchor["A"] == anchor2_id:
                a2_range = float(anchor["R"])
                node_count += 1

        if node_count == 2:
            tag_pos = calc_tag_pos(a1_range, a2_range)
            print(tag_pos)
            render_data(screen, tag_pos)
        time.sleep(0.1)

    

if __name__ == '__main__':
    main()