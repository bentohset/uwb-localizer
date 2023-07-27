# Receives N number of tags via UDP datagram (connectionless)

"""
TODO: Animate gantry opening and closing when going near
"""

import time
import socket
import json
import pygame
from localization import trilateration
from tests import Test
from utils import convert_to_mac, wrapText

# toggle to use tests generated from imported tests.py
TESTING = True

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
# receives in format: {"links":{"A":"41","R":"0.17", "T": "42", "Payload":"JDOSA212..."}}
# return in format: {"A":"41 or 42","R":"0.17", "T": "42", "Payload":"JJASD..."}
# each link represents a tag
def read_data(sock):
    uwb_list = {}
    data, addr = sock.recvfrom(1024)

    line = data.decode()

    print(line)
    json_string = b'{"links":{"A":"42","R":"-2750.36","T":"428800CADE564557","Payload":"002DCF462904B478D868A7FF3F2BF1FCD97A96092CA5577464C4AF1528A4E957DB5E20FB38A84EA6149325562444DF598D437BBE9016899D7E77C62F269888F5B430D4349D3A0D0FBD2FA1F70FD968F4D93CA835E58F2ACDC240C1CF52716A722923B05B"}}'

    uwb_data = json.loads(line)

    uwb_list = uwb_data["links"]

    return uwb_list

# scales the distances in meters to pixels according to screen dimensions
def scale_to_pixel(pos):
    #convert to pixel
    return (pos * pixel_scale)

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
    # tags_pos = array of objects wrt each tag and coordinate
    # renders each tag by looping thru array
    for i in range(0, len(tags_pos)):
        tag_id = tags_pos[i]["tagID"]

        tagpos = tags_pos[i]["tagpos"]
        tagdist = tags_pos[i]["tagDist"]
        payload = tags_pos[i]["payload"]
        text_surface3 = font.render('TagID: '+convert_to_mac(str(tag_id)), False, white, black)
        text_dist1 = font.render(str(tagdist[0]) + 'm', False, white, black)
        text_dist2 = font.render(str(tagdist[1]) + 'm', False, white, black)
        line1_ctr = ((tagpos[0] + anchor1_pos[0])//2 - 80, (tagpos[1] + anchor1_pos[1])//2)
        line2_ctr = ((tagpos[0] + anchor2_pos[0])//2 + 40, (tagpos[1] + anchor2_pos[1])//2)
        
        # if tagpos[1] < 100 and TESTING:
        # extra = drawText(screen, text_payload_str, white, text_payload_box, font, 0, None)
        # if extra:
        #     print(extra)
        """Render different tag_id payload on different lines
        When tag_id not in tags_pos, dont render/remove render
        """
        if tagpos[1] < 100:
            text_payload_str = convert_to_mac(str(tag_id))+" payload: "+str(payload)
            wrapText(text_payload_str, 5,800, white, 1390, font, screen)
        
        pygame.draw.line(screen, white, tagpos, anchor1_pos)
        pygame.draw.line(screen, white, tagpos, anchor2_pos)
        screen.blit(text_surface3, (tagpos[0]+20, tagpos[1]))
        screen.blit(text_dist1, line1_ctr)
        screen.blit(text_dist2, line2_ctr)
        # screen.blit(text_payload, (5,850))
        
        pygame.draw.circle(screen, green, tagpos, 14)

    pygame.display.flip()


# gets the index of the object within the array corresponding to the tagID
def find_obj_index(ds, target):
    for i, obj in enumerate(ds):
        if obj["tagID"] == target:
            return i
    return -1

# checks if the object corresponding to the tag_id is in the array
# updates it if exists, else appends the object to the array in place
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
            list = test.generate_test_approaching_one()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                print("Exit")
                running = False
                pygame.quit()

        # {"A":"41","R":"0.17", "T": "42", "Payload":"HSIODH2813..."}
        if not list:
            continue
        tag_id = list["T"]

        # array of objects [{T: 42, A1: 0.4, A2: 0.5, payload:fadsgdsd...., count: 1}, {T: 42, A1: 0.4, A2: 0.3, payload:asdafsa, count: 2}]
        # find index of ds with T == tag_id
        indx =  find_obj_index(data_struct, tag_id)  # gets the index of the object within the array corresponding to the tagID
        # if not found, create a new instance
        if (indx == -1):
            data_struct.append({
                "tagID": tag_id,
                "A1": 0,
                "A2": 0,
                "payload": "",
                "count1": 0,
                "count2": 0
            })
            indx = len(data_struct)-1
        # add to the a1/a2 ranges and update count

        # for all objects in the array if count == 2, render data on screen
        if list["A"] == anchor1_id:
            a1_range = float(list["R"])
            data_struct[indx]["A1"] = a1_range
            data_struct[indx]["payload"] = str(list["Payload"])
            data_struct[indx]["count1"] = 1

        elif list["A"] == anchor2_id:
            a2_range = float(list["R"])
            data_struct[indx]["A2"] = a2_range
            data_struct[indx]["payload"] = str(list["Payload"])
            data_struct[indx]["count2"] = 1
        
        # runs through the data structure. 
        # if both tags have readings existing (signalled by count1 and count2), calculate coordinates and create object
        # appends the object in tags_pos
        for i in range(0, len(data_struct)):
            if data_struct[i]["count1"] == 1 and data_struct[i]["count2"] == 1:

                tag_pos = trilateration(scale_to_pixel(data_struct[i]["A1"]), scale_to_pixel(data_struct[i]["A2"]), anchor1_pos, anchor2_pos)
                
                # ensures that tag positions are positive. intersection of range radius may result in 2 positions positive and negative, take the positive always
                positive_tag_pos = [subarr for subarr in tag_pos if all(element >= 0 for element in subarr)][0]     

                new_object = {
                    "tagpos": (positive_tag_pos[0], positive_tag_pos[1]),
                    "tagDist": (data_struct[i]["A1"], data_struct[i]["A2"]),
                    "tagID": data_struct[i]["tagID"],
                    "payload": data_struct[i]["payload"],
                }

                check_and_update(tags_pos, tag_id, new_object)

                # reset count (wait for next readings)
                data_struct[i]["count1"] = 0
                data_struct[i]["count2"] = 0

        # renders coordinates of the tag(s)
        render_data(screen, font, tags_pos)
        
        if TESTING:
            time.sleep(0.5)
        else:
            time.sleep(0.05)

        print("")

    

if __name__ == '__main__':
    main()