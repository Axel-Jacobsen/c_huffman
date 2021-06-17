#! /usr/bin/env python3

import sys
import networkx

from typing import Tuple


class Node:
    def __init__(self, m, l, r, tkn, cnt, il):
        self.m: str = m
        self.l: str = l
        self.r: str = r
        self.tkn: str = tkn
        self.cnt: int = cnt
        self.is_leaf: bool = il

    def __repr__(self):
        return f"me: {self.m}  l: {self.l}  r: {self.r}  tkn: {self.tkn}  cnt: {self.cnt}  il: {self.is_leaf}"


def create_node(s: str) -> Tuple[str, Node]:
    splitted = s.split("  ")
    ME = splitted[0].replace("ME: ", "")
    L = splitted[1].replace("L: ", "")
    R = splitted[2].replace("R: ", "")
    TKN = splitted[3].replace("TKN: ", "")
    CNT = splitted[4].replace("count: ", "")
    IL = splitted[5].replace("is_leaf: ", "")
    return ME, Node(ME, L, R, TKN, int(CNT), IL == "1")


def get_root_ptr(ptr_to_node):
    mx = 0
    mx_ptr = ""
    for ptr, node in ptr_to_node.items():
        if node.cnt > mx:
            mx = node.cnt
            mx_ptr = ptr
    return mx_ptr


if len(sys.argv) != 2:
    fname = "node_init_list.txt"
    print("gib node init list file path")
    exit(1)
else:
    fname = sys.argv[1]

raw_dat = open(fname, "r").read()
dat = raw_dat.split("\n")
ptr_to_Node = dict([create_node(line) for line in dat if line != ""])


def print_all():
    for k, v in ptr_to_Node.items():
        print(v)


edgs = []
def create_edges(N):
    global edgs
    if N.l != "0x0":
        edgs.append((N.m, N.l))
        create_edges(ptr_to_Node[N.l])
    if N.r != "0x0":
        edgs.append((N.m, N.r))
        create_edges(ptr_to_Node[N.r])

root_node_ptr = get_root_ptr(ptr_to_Node)
create_edges(ptr_to_Node[root_node_ptr])

G = networkx.Graph()
G.add_edges_from(edgs)

import matplotlib.pyplot as plt

try:
    cycle = networkx.find_cycle(G, source=root_node_ptr)
    print(f"CYCLES!!!")
    for edge in cycle:
        print(edge)
except networkx.exception.NetworkXNoCycle:
    print("No Cycle :)")

networkx.draw_planar(G, node_size=10)
plt.show()
