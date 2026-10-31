#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Wildfire ROS Model - Rothermel 

Description:
This module contains the implementation of the Balbi 2020 for predicting the Rate of Spread (ROS) of wildfires.
It is based on  https://www.fs.fed.us/rm/pubs_series/rmrs/gtr/rmrs_gtr371.pdf   
Rothermel, Richard C. 1972. A mathematical model for predicting fire spread in wildland fuels. Res. Pap. INT-115. Ogden, UT: U.S. Department of Agriculture, Intermountain Forest and Range Experiment Station- https://www.fs.usda.gov/treesearch/pubs/32533

equation set from :
Andrews, Patricia L. 2018. The Rothermel surface fire spread model and associated developments: A comprehensive explanation. Gen. Tech. Rep. RMRS-GTR-371. Fort Collins, CO: U.S. Department of Agriculture, Forest Service, Rocky Mountain Research Station


Author: Jean-Baptiste Filippi
Organization: CNRS
License: GPL

sources inspired from http://www.prairieprojectknowledgehub.org/books/fire/page/rothermels-simple-fire-model

Usage:
from models.[model_file_name] import [ModelClassName]

"""
import math
import numpy as np
import matplotlib.pyplot as plt

from .model_set import *


def Rothermel1972_valuesset():
    return {
        "identification": {"CODE": "A4"},
        "typical": {
            "H": 18608.0,
            "SAVcar": 5705.380577427821,
            "fd": 1.8,
            "fuelDens": 512.592,
            "Dme": 0.2,
        },
        "fuelstate": {"fl1h_tac": 0.4},
        "environment": {"wind": 10, "slope": 0, "mdOnDry1h": 0.06},
        "model": {"totMineral": 0.0555, "effectMineral": 0.01},
        "constants": {},
    }


def Rothermel1972(Z, print_calculus=False):

    sa_vol_ratio = Z.SAVcar_ftinv  # Fuel Particle surface area to volume ratio (1/ft)
    fuel_load = Z.fl1h_lbft2  # Ovendry fuel loading
    bed_depth = Z.fd_ft  # Fuel depth (ft)
    dead_extinction_moisture = Z.Dme_r  # Moisture content of extinction
    moisture_content = Z.mdOnDry1h_r  # Fuel particle moisture content
    wind = Z.wind_ftmin  # wind velocity at mid flame
    slope = math.radians(Z.slope_deg)  # slope angle
    heat_content = Z.H_BTUlb  # Fuel particle low heat content
    mineral_content = Z.totMineral_r  # Fuel Particle effective mineral content
    effective_mineral_content = (
        Z.effectMineral_r
    )  # Fuel Particle effective mineral content
    particle_density = Z.fuelDens_lbft3  # Ovendry particle density

    tan_slope = math.tan(slope)  #  in radians
    preignition = 250 + 1116 * moisture_content

    heating_number = np.exp(-138 / sa_vol_ratio)

    bulk_density = fuel_load / bed_depth
    packing_ratio = bulk_density / particle_density
    optimal_packing = 3.348 * sa_vol_ratio**-0.8189

    slope_factor = 5.275 * packing_ratio**-0.3 * tan_slope**2

    C = 7.74 * np.exp(-0.133 * sa_vol_ratio**0.55)
    B = 0.02526 * sa_vol_ratio**0.54
    E = 0.715 * np.exp(sa_vol_ratio * -3.59e-4)
    wind_factor = C * wind**B * (packing_ratio / optimal_packing) ** E

    propagating_flux = (192 + 0.2595 * sa_vol_ratio) ** -1 * np.exp(
        (0.792 + 0.681 * sa_vol_ratio**0.5) * (packing_ratio + 0.1)
    )

    mineral_dampening = min(1.0, 0.174 * effective_mineral_content**-0.19)

    rm = min(1.0, moisture_content / dead_extinction_moisture)
    moisture_dampening = 1 - 2.59 * rm + 5.11 * rm**2 - 3.52 * rm**3

    net_fuel_load = fuel_load * (1 - mineral_content)

    max_reaction = sa_vol_ratio**1.5 * (495 + 0.0594 * sa_vol_ratio**1.5) ** -1

    A = 133 * sa_vol_ratio**-0.7913

    optimal_reaction = (
        max_reaction
        * (packing_ratio / optimal_packing) ** A
        * np.exp(A * (1 - packing_ratio / optimal_packing))
    )

    reaction_intensity = (
        optimal_reaction
        * net_fuel_load
        * heat_content
        * moisture_dampening
        * mineral_dampening
    )

    rate_of_spread = (
        reaction_intensity
        * propagating_flux
        * (1 + wind_factor + slope_factor)
        / (bulk_density * heating_number * preignition)
    )

    return {
        "ROS_ftmin": rate_of_spread,
        "PR_r": propagating_flux,
        "FI_BTUftmin": reaction_intensity,
    }
