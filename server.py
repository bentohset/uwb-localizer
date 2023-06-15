# Receives N number of tags via UDP datagram (connectionless)

import time
import socket
import json
import pygame
from localization import trilateration
from tests import Test

TESTING = False

# TODO: 
# explore apple device
# check if DW1000 compatibe

######Calibration/Structure Setup########
anchor1_pos = [500, 0]
anchor2_pos = [900, 0]
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
yellow = (255, 255, 0)
window_width = 1400
window_height = 900

# creates pygame instance and font for text
def init_game():
    pygame.init()
    window = pygame.display.set_mode((window_width, window_height), pygame.RESIZABLE)
    clock = pygame.time.Clock()
    pygame.font.init()
    my_font = pygame.font.SysFont('Arial', 28)
    return window, clock, my_font

# creates socket: UDP datagram
def init_udp():
    hostname = socket.gethostname()
    UDP_IP = socket.gethostbyname(hostname)
    print("***Local ip:" + str(UDP_IP) + "***")
    UDP_PORT = 80
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print("Socket created")

    return sock


# gets data from connectionless socket, identifies device id and creates object
# receives in format: {"links":{"A":"41","R":"0.17", "T": "42"}}
# return in format: {"A":"41","R":"0.17", "T": "42"}
def read_data(sock):
    uwb_list = {}
    data, addr = sock.recvfrom(1024)
    # print(addr)
    # print(data)
    line = data.decode()

    # print(line)
    uwb_data = json.loads(line)

    uwb_list = uwb_data["links"]

    print(uwb_list)
    print("")

    return uwb_list


# insert ':' between every 2 digits to show on display as MAC address
def convert_to_mac(val):
    res = ''
    for idx in range(0,len(val)):
        res += val[idx]
        if idx % 2 == 1 and idx != 0 and idx != len(val)-1:
            res += ':'

    return res

# scales the distances in meters to pixels according to screen dimensions
def scale_to_pixel(pos):
    #convert to pixel
    return (pos * pixel_scale)
    return (pos[0] * pixel_scale, pos[1] * pixel_scale)

# renders data on the pygame screen
def render_data(screen, font, tags_pos):
    screen.fill((0, 0, 0))
    # Render the positions of the anchors statically
    text_surface1 = font.render('Anchor 1/ Gantry', False, white, black)
    text_surface2 = font.render('Anchor 2/ Gantry', False, white, black)
    pygame.draw.circle(screen, white, anchor1_pos, 14)  # Red anchor
    pygame.draw.circle(screen, white, anchor2_pos, 14)  # Green anchor
    pygame.draw.line(screen, yellow, anchor1_pos, (500,900))
    pygame.draw.line(screen, yellow, anchor2_pos, (900,900))
    screen.blit(text_surface1, (anchor1_pos[0]-200, anchor1_pos[1]+10))
    screen.blit(text_surface2, (anchor2_pos[0]+40, anchor1_pos[1]+10))
    print("tagss")
    print(tags_pos)

    # tags_pos -> array of objects wrt each tag and coordinate
    # renders each tag by looping thru array
    for i in range(0, len(tags_pos)):
        tag_id = tags_pos[i]["tagID"]
        tagpos = tags_pos[i]["tagpos"]
        tagdist = tags_pos[i]["tagDist"]
        text_surface3 = font.render('TagID: '+convert_to_mac(str(tag_id)), False, white, black)
        text_dist1 = font.render(str(tagdist[0]) + 'm', False, white, black)
        text_dist2 = font.render(str(tagdist[1]) + 'm', False, white, black)
        line1_ctr = ((tagpos[0] + anchor1_pos[0])//2 - 80, (tagpos[1] + anchor1_pos[1])//2)
        line2_ctr = ((tagpos[0] + anchor2_pos[0])//2 + 40, (tagpos[1] + anchor2_pos[1])//2)
        
        pygame.draw.line(screen, white, tagpos, anchor1_pos)
        pygame.draw.line(screen, white, tagpos, anchor2_pos)
        screen.blit(text_surface3, (tagpos[0]+20, tagpos[1]))
        screen.blit(text_dist1, line1_ctr)
        screen.blit(text_dist2, line2_ctr)
        
        pygame.draw.circle(screen, green, tagpos, 14)

    pygame.display.flip()


# gets the index of the object within the array corresponding to the tagID
def find_obj_index(ds, target):
    for i, obj in enumerate(ds):
        if obj["tagID"] == target:
            return i
    return -1

# checks if the object corresponding to the tag_id is in the array
# updates it if exists, else appends
def check_and_update(array, tag_id, new_object):
    print(array)
    for i, obj in enumerate(array):
        if obj["tagID"] == tag_id:
            array[i] = new_object
            return
    array.append(new_object)


def main():
    if not TESTING:
       sock = init_udp()
    if TESTING:
        test = Test()
    screen, clock, font = init_game()
    print("game initialized, waiting to receive")
    a1_range = 0.00
    a2_range = 0.00
    running = True
    data_struct = []    #array of objects fields: T, A1, A2, count
    tags_pos = []

    while running:
        test_data = 0
        list = {}
        
        if not TESTING:
            list = read_data(sock)
        else:
            print("testing")
            list = test.generate_test_rand_two()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                print("Exit")
                running = False
                pygame.quit()

        # {"A":"41","R":"0.17", "T": "42"}
        if not list:
            continue
        tag_id = list["T"]

        # array of objects [{T: 42, A1: 0.4, A2: 0.5, count: 1}, {T: 42, A1: 0.4, A2: 0.3, count: 2}]
        # find index of ds with T == tag_id
        indx =  find_obj_index(data_struct, tag_id) 
        if (indx == -1):
            data_struct.append({
                "tagID": tag_id,
                "A1": 0,
                "A2": 0,
                "count1": 0,
                "count2": 0
            })
            indx = len(data_struct)-1
        # add to the a1/a2 ranges and update count

        # for all objects in the array if count == 2, render data on screen
        if list["A"] == anchor1_id:
            a1_range = float(list["R"])
            data_struct[indx]["A1"] = a1_range
            data_struct[indx]["count1"] = 1

        elif list["A"] == anchor2_id:
            a2_range = float(list["R"])
            data_struct[indx]["A2"] = a2_range
            data_struct[indx]["count2"] = 1
        
        # print("before adding", data_struct)
        for i in range(0, len(data_struct)):
            if data_struct[i]["count1"] == 1 and data_struct[i]["count2"] == 1:
                # print("data", data_struct)
                tag_pos = trilateration(scale_to_pixel(data_struct[i]["A1"]), scale_to_pixel(data_struct[i]["A2"]), anchor1_pos, anchor2_pos)
                positive_tag_pos = [subarr for subarr in tag_pos if all(element >= 0 for element in subarr)][0]

                new_object = {
                    "tagpos": (positive_tag_pos[0], positive_tag_pos[1]),
                    "tagDist": (data_struct[i]["A1"], data_struct[i]["A2"]),
                    "tagID": data_struct[i]["tagID"]
                }
                # print("newobj", new_object)
                check_and_update(tags_pos, tag_id, new_object)
                # print("tags_pos", tags_pos)
                data_struct[i]["count1"] = 0
                data_struct[i]["count2"] = 0

        render_data(screen, font, tags_pos)
        
        time.sleep(0.05)
        print("")

    

if __name__ == '__main__':
    main()