import os
import pickle as pkl
import json
import struct
import numpy as np


def save_to_pkl(x, file):
    folder = "/".join(file.split("/")[:-1])
    if not os.path.exists(folder):
        os.makedirs(folder)
    with open(file, "wb") as f:
        pkl.dump(x, f)


def save_to_json(x, file):
    folder = "/".join(file.split("/")[:-1])
    if not os.path.exists(folder):
        os.makedirs(folder)
    with open(file, "w") as f:
        json.dump(x, f, indent=4)


def load_pkl(file):
    with open(file, "rb") as f:
        x = pkl.load(f)
    return x

