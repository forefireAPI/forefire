#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Wildfire ROS Model - Balbi2020

Description:
This module contains the implementation of the Balbi 2020 for predicting the Rate of Spread (ROS) of wildfires.
It is based on Balbi et. al. IJWF 2020 A convective–radiative propagation model for wildland fires 10.1071/WF19103

Author: Jean-Baptiste Filippi
Organization: CNRS
License: GPL

Usage:
from models.[model_file_name] import [ModelClassName]

"""
import math
import numpy as np


from .model_set import *


def Balbi2020_valuesset():
    return {
        "identification": model_parameters({"CODE": "PN1"}),
        "typical": model_parameters(
            {"H": 17400.0, "Cpf": 2030, "SAV1h": 6000, "fd": 0.1, "fuelDens": 500}
        ),
        "fuelstate": model_parameters({"fl1h": 0.4, "mdOnDry1h": 0.1}),
        "environment": model_parameters(
            {"Ta": 300, "wind": 0, "slope": 0.0, "airDens": 1.225}
        ),
        "model": model_parameters(
            {
                "Ti": 600,
                "Tvap": 373,
                "Tau0": 75591,
                "hEvap": 2300.0,
                "Cpa": 1150,
                "X0": 0.3,
                "K1": 130,
                "r00": 2.5e-05,
                "st": 17,
            }
        ),
        "constants": model_parameters({"B": 5.6e-08, "g": 9.81}),
    }


def Balbi2020(Z, print_calculus=False):

    # Fuel Characteristic Parameters
    lDeltaH = Z.H_Jkg
    lh = Z.fd_m
    lrhov = Z.fuelDens_kgm3
    st = Z.st_r
    ltau0 = Z.Tau0_spm
    lCp = Z.Cpf_JkgK
    ls = Z.SAV1h_minv

    # Fuel State parameter
    lsigma = Z.fl1h_kgm2
    lm = Z.mdOnDry1h_r

    # Environment parameters
    lTa = Z.Ta_degK
    lalpha = math.radians(Z.slope_deg)
    RU = Z.wind_mps
    lrhoa = Z.airDens_kgm3

    # Model Parameters
    lDeltah = Z.hEvap_Jkg
    Tvap = Z.Tvap_degK
    Cpa = Z.Cpa_JkgK
    lTi = Z.Ti_degK

    # Model fitted parameters
    K1 = Z.K1_spm
    lr00 = Z.r00
    lChi0 = Z.X0

    # Constants
    B = Z.B
    lg = Z.g

    if lh <= 0:
        return 0

    lU = 0

    if RU > 0:
        lU = RU

    R = 0.1  # first guess in iteration
    Rnew = 0
    maxEps = 0.001
    N = 100
    step = 1
    stopcondition = True

    flag = 1
    error = 0

    # Packing ratio
    Beta = lsigma / (lh * lrhov)
    # Leaf Area ratio just before eq. 13 S is the total fuel surface area per horizontal area unit of fuel bed and denotes the double of the leaf area index (LAI)
    S = ls * Beta * lh
    # Ignition energy (J/kg) # eq. 9
    q = lCp * (lTi - lTa) + lm * (lDeltah + lCp * (Tvap - lTa))
    # scaling factor eq. 17
    ar = min(S / (2 * math.pi), 1.0)
    # Radiative factor # eq. 16
    A = ar * ((lChi0 * lDeltaH) / (4 * q))
    # coefficient p required for T derived from expression between C7 and C8
    p = (2 / lr00) / ltau0

    while stopcondition:

        # Radiant fractor eq. C7
        Chi = lChi0 / (1 + p * ((R * ltau0 * math.cos(lalpha)) / (2 * ls)))
        # Mean Flame Temperature eq. B11
        T = lTa + lDeltaH * ((1 - Chi) / (Cpa * (st + 1)))
        # reference vertical velocity eq. B9
        u0 = 2 * (st + 1) / ltau0 * T / lTa * lrhov / lrhoa * min(S, 2 * math.pi)
        # flame angle
        gamma = math.atan(math.tan(lalpha) + (lU / u0))
        # Flame Height
        H = (u0**2) / (lg * (T / lTa - 1.0))

        Rb = min((S / math.pi), 1.0) * ((B * (T**4)) / (Beta * lrhov * q))
        Rc1 = ls * (lDeltaH / (q * ltau0)) * min(lh, (2 * math.pi) / (ls * Beta))

        Rc2 = (lh / (2 * lh + H)) * math.tan(lalpha) + (
            (lU * math.exp(-K1 * pow(Beta, 0.5) * R)) / u0
        )

        Rc = Rc1 * Rc2  # eq. 27

        Rr = (
            A
            * R
            * (
                (1 + math.sin(gamma) - math.cos(gamma))
                / (1 + ((R * math.cos(gamma)) / (ls * lr00)))
            )
        )  # eq. 15

        Rnew = Rb + Rc + Rr

        error = R - Rnew

        R = Rnew
        if step > N:
            flag = 0
            break
        step = step + 1

        stopcondition = abs(error) > maxEps

    if flag != 1:
        if print_calculus:
            print(
                f"no convergence in {N} steps for Balbi, error is {error}, returning ROS ",
                Z,
            )
    # else:
    #     print(".", end="")

    return {"ROS_mps": Rnew, "FllH_m": H}


def Balbi2011(Z, print_calculus=False):

    # Fuel Characteristic Parameters
    lDeltaH = Z.H_Jkg
    lh = Z.fd_m
    lrhov = Z.fuelDens_kgm3
    st = Z.st_r
    ltau0 = Z.Tau0_spm
    lCp = Z.Cpf_JkgK

    # Model Parameters
    lDeltah = Z.hEvap_Jkg
    Tvap = Z.Tvap_degK
    Cpa = Z.Cpa_JkgK
    lTi = Z.Ti_degK

    # Model fitted parameters
    K1 = Z.K1_spm
    lr00 = Z.r00
    lChi0 = Z.X0

    # Constants
    B = Z.B
    lg = Z.g

    # Fuel State parameter
    ls = Z.SAV1h_minv
    lsigma = Z.fl1h_kgm2
    lm = Z.mdOnDry1h_r

    # Environment parameters
    lTa = Z.Ta_degK
    lalpha = Z.slope_rad
    RU = Z.wind_mps
    lrhoa = Z.airDens_kgm3

    lRhod = Z.fuelDens_kgm3
    lRhol = valueOf["Rhol"]
    lMd = valueOf["Md"]
    lMl = valueOf["Ml"]
    lsd = valueOf["sd"]
    lsl = valueOf["sl"]
    le = valueOf["e"]
    lSigmad = valueOf["Sigmad"]
    lSigmal = valueOf["Sigmal"]
    lstoch = valueOf["stoch"]
    lRhoA = valueOf["RhoA"]
    lTa = valueOf["Ta"]
    lTau0 = valueOf["Tau0"]
    lDeltah = valueOf["Deltah"]
    lDeltaH = valueOf["DeltaH"]
    lCp = valueOf["Cp"]
    lTi = valueOf["Ti"]
    lX0 = valueOf["X0"]
    lr00 = valueOf["r00"]
    lai = valueOf["Blai"]

    cosCurv = 1

    if le <= 0:
        return 0

    Betad = lSigmad / (le * lRhod)
    Betal = lSigmal / (le * lRhol)
    Sd = lsd * le * Betad
    Sl = lsl * le * Betal
    nu = min((Sd) / lai, 1)
    normal_wind = adjustementWind * valueOf["normalWind"]
    B = 5.670373e-8
    a = lDeltah / (lCp * (lTi - lTa))
    r0 = lsd * lr00
    A0 = (lX0 * lDeltaH) / (4 * lCp * (lTi - lTa))
    xsi = (lMl - lMd) * ((Sd / Sl) * (lDeltah / lDeltaH))  # cf. Santoni et al., 2011
    A = cosCurv * (nu * A0 / (1 + a * lMd)) * (1 - xsi)
    T = lTa + (lDeltaH * (1 - lX0) * (1 - xsi)) / ((lstoch + 1) * Cpa)
    R00 = (B * T**4) / (lCp * (lTi - lTa))
    R0 = (le / lSigmad) * (R00) / (1 + a * lMd) * Sd / (Sd + Sl) * Sd / (Sd + Sl)
    u00 = (2 * lai * (lstoch + 1) * T * lRhod) / (lRhoA * lTa * lTau0)
    u0 = nu * u00

    tanGamma = adjustementSlope * valueOf["slope"] + (normal_wind / u0)
    gamma = atan(tanGamma)

    if gamma > 0:
        geomFactor = r0 / cos(gamma) * (1 + sin(gamma) - cos(gamma))
        Rt = R0 + A * geomFactor - r0 / cos(gamma)
        R = 0.5 * (Rt + sqrt(Rt * Rt + 4.0 * r0 * R0 / cos(gamma)))
    else:
        R = R0

    return R
