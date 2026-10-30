#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Wildfire ROS Model - Mc Arthur

Description:
This module contains the implementation of 

Cruz, MG, Cheney, NP, Gould, JS, McCaw, WL, Kilinc, M, Sullivan, AL (2022) An empirical-based model for predicting the forward spread rate of wildfires in eucalypt forests. International Journal of Wildland Fire

Cruz, MG (2021) The Vesta Mk 2 rate of fire spread model: a user’s guide. CSIRO Client Report No. EP2021-2731, Canberra.

see https://research.csiro.au/amicus/resources/model-library/

Author: Jean-Baptiste Filippi
Organization: CNRS
License: GPL

Usage:
from models.[model_file_name] import [ModelClassName]

"""
import math
import numpy as np
import matplotlib.pyplot as plt


from .model_set import *
