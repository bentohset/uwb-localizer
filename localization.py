import math
import cmath
import numpy as np
from scipy.optimize import least_squares

# anchor1_pos = [200, 0]
# anchor2_pos = [400, 0]

# simple calculation between of coordinates using cosine rule
# issue: not as accurate, might produce imaginary number
def calc_tag_pos(a1, a2, anchor_dist):
    res = [0,0]
    try:
        cos_a = (a1 * a1 + anchor_dist*anchor_dist - a2 * a2) / (2 * a1 * anchor_dist)
        x = a1 * cos_a
        y = a1 * math.sqrt(1 - cos_a * cos_a)
        res = [round(x,2), round(y,2)]

    except:
        res = [0,0]

    return res

# calculates the intersection between the radius of the 2 distances
# returns the positive subarray of the 2 coordinate intersection
def trilateration(a1, a2, anchor1_pos, anchor2_pos):
    #in pixels
    d = cmath.sqrt((anchor1_pos[0] - anchor2_pos[0]) ** 2 + (anchor1_pos[1]-anchor2_pos[1])**2)
    l = ((a1 ** 2) - (a2 ** 2) + (d ** 2)) / (2 * d)
    h = cmath.sqrt((a1**2) - (l**2))
    coordinates = np.zeros((2,2))
    # 2 points at which the 2 circles intersect

    factor_1 = ((l/d) * (anchor2_pos[0]-anchor1_pos[0])) + anchor1_pos[0]
    factor_2 = ((h/d) * (anchor2_pos[1]-anchor1_pos[1]))
    
    factor_3 = ((l/d) * (anchor2_pos[1]-anchor1_pos[1])) + anchor1_pos[1]
    factor_4 = ((h/d) * (anchor2_pos[0]-anchor1_pos[0]))

    coordinates[0] = round((factor_1 + factor_2).real, 1), round((factor_3 - factor_4).real, 1)
    coordinates[1] = round((factor_1 - factor_2).real, 1), round((factor_3 + factor_4).real, 1)

    return coordinates

## least squares testing
def least_squares_estimation(a1, a2, anchor1_pos, anchor2_pos):
    def objective(x):
        tag_pos = np.array(x[0], x[1])
        residual1 = np.linalg.norm(tag_pos - anchor1_pos) - a1
        residual2 = np.linalg.norm(tag_pos - anchor2_pos) - a2
        return [residual1, residual2]
    initial_guess = np.array([317.2, 93.6])
    result = least_squares(objective, initial_guess)
    print(result)
    estimated = result.x
    return estimated


def solve_2D(a1, a2, anchor1_pos, anchor2_pos):
    distances = [a1, a2]
    anchors = [anchor1_pos, anchor2_pos]
    n_anchors = 2
    n_coordinates = 2
    A = np.zeros((n_anchors-1, n_coordinates))
    b = np.zeros(n_anchors-1)
    i = 1
    k_l = anchors[0][0] ** 2 + anchors[0][1] ** 2
    while i < n_anchors:
        A[i-1][0] = anchors[i][0] - anchors[0][0]
        A[i-1][1] = anchors[i][1] - anchors[0][1]

        k_cur = anchors[i][0] ** 2 + anchors[i][1] ** 2
        b[i-1] = distances[0] ** 2 - distances[i] ** 2 - k_l + k_cur
        i += 1
    return A, b

def least_square(a1, a2, anchor1_pos, anchor2_pos):
    A, b = solve_2D(a1, a2, anchor1_pos, anchor2_pos)
    A_T = np.transpose(A)
    product_1 = np.dot(A_T, A)
    inverse = np.linalg.inv(product_1)
    product_2 = np.dot(A_T, b)
    r = np.dot(inverse, product_2)

    return r/2