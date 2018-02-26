"""
Represents and generates positions
"""
from random import random

class Rectangle():
    def __init__(self, origin, width, height):
        self.origin = origin
        self.width = width
        self.height = height

    def contains(self, point):
        return point[0] >= self.origin[0] and point[0] <= self.origin[0]+self.width and\
                point[1] >= self.origin[1] and point[1] <= self.origin[1]+self.height

    def new_interior_point(self):
        """
        Generates a random position in the rectangle
        formed by top_position, oof width and height
        """
        return (self.origin[0] + int(random()*self.width),
               (self.origin[1] + int(random()*self.height)))

    def __repr__(self):
        return "(%d.%d: %d %d)" % (self.origin[0], self.origin[1], self.width, self.height)
