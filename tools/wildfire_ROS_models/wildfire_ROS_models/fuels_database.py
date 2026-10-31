#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Fuel Model Tables data

Description:
This module contains values for parameters as of some databases such as :
- Scott, Joe H. and Robert E. Burgan. "Standard Fire Behavior Fuel Models: A Comprehensive Set for Use with Rothermel's Surface Fire Spread Model." USDA Forest Service. General Technical Report RMRS-GTR-153. 2005
- Andrews, Patricia L. 2018. The Rothermel surface fire spread model and associated developments: A comprehensive explanation. Gen. Tech. Rep. RMRS-GTR-371. Fort Collins, CO: U.S. Department of Agriculture, Forest Service, Rocky Mountain Research Station
- Balbi et. al. IJWF 2020 A convective–radiative propagation model for wildland fires 10.1071/WF19103

data may be imported as csv files, here stored as variables. 
All variables are post-fixed with the units such as defined in the original paper, 
corresponding to the units shortname in model_set.py
    
convention for variable names such as in parameter keys

Author: Jean-Baptiste Filippi
Organization: CNRS
License: GPL
"""


# from Andrews and Rothermel 2017
AR2017_table_csv = """
CODE,ftype,fl1h_tac,fl10h_tac,fl100h_tac,flLherb_tac,flLwood_tac,SAV1h_ftinv,SAVLDherb_ftinv,SAVLwood,fd_ft,Dme_pc,SAVcar_ftinv,bulkDens_lbft3,packRatio_r
A4,S,5.0,4.0,2.0,0,5.0,2000,0,1500,6.0,20,1739,0.12,0.52
SH5,S,3.6,2.1,0,0,2.9,750,0,1600,6.0,15,1252,0.007,0.21
SH7,S,3.5,5.3,2.2,0,3.4,750,0,1600,6.0,15,1233,0.11,0.35
SH9,D,4.5,2.45,0,1.55,7.0,750,1800,1500,4.4,40,1378,0.16,0.56"""
# parameters valid for all fuel models in table
AR2017_anyfueltable_csv = """SAV10h_ftinv,SAV100h_ftinv,H_BTUlb,fuelDens_lbft3,totMineral_r,effectMineral_r
109,30,8000,32,0.0555,0.01
"""

pineNeedlesBalbi2020_csv = """CODE,Ta_degK,Ti_degK,Tvap_degK,Tau0_spm,hEvap_Jkg,H_Jkg,Cpf_JkgK,Cpa_JkgK,X0,K1_spm,st_r,r00,B,g,fl1h_kgm2,SAV1h_minv,fd_m,mdOnDry1h_r,fuelDens_kgm3,airDens_kgm3
PN1,300,600,373,75591,2300000.0,17400000.0,2030,1150,0.3,130,17,2.5e-05,5.6e-08,9.81,0.4,6000,0.1,0.1,500,1.225"""


# values as in table 7 in  Scott&Burgan 2005. but they differ from the text values
CB2005_t7_csv = """CODE,fl1h_tac,fl10h_tac,fl100h_tac,flLherb_tac,flLwood_tac,ftype,SAV1h_ftinv,SAV10h_ftinv,SAV100h_ftinv,fd_ft,Dme_pc,H_BTUlb
GR1,0.10,0.00,0.00,0.30,0.00,D,2200,2000,9999,0.4,15,8000
GR2,0.10,0.00,0.00,1.00,0.00,D,2000,1800,9999,1.0,15,8000
GR3,0.10,0.40,0.00,1.50,0.00,D,1500,1300,9999,2.0,30,8000
GR4,0.25,0.00,0.00,1.90,0.00,D,2000,1800,9999,2.0,15,8000
GR5,0.40,0.00,0.00,2.50,0.00,D,1800,1600,9999,1.5,40,8000
GR6,0.10,0.00,0.00,3.40,0.00,D,2200,2000,9999,1.5,40,9000
GR7,1.00,0.00,0.00,5.40,0.00,D,2000,1800,9999,3.0,15,8000
GR8,0.50,1.00,0.00,7.30,0.00,D,1500,1300,9999,4.0,30,8000
GR9,1.00,1.00,0.00,9.00,0.00,D,1800,1600,9999,5.0,40,8000
GS1,0.20,0.00,0.00,0.50,0.65,D,2000,1800,1800,0.9,15,8000
GS2,0.50,0.50,0.00,0.60,1.00,D,2000,1800,1800,1.5,15,8000
GS3,0.30,0.25,0.00,1.45,1.25,D,1800,1600,1600,1.8,40,8000
GS4,1.90,0.30,0.10,3.40,7.10,D,1800,1600,1600,2.1,40,8000
SH1,0.25,0.25,0.00,0.15,1.30,D,2000,1800,1600,1.0,15,8000
SH2,1.35,2.40,0.75,0.00,3.85,N/A,2000,9999,1600,1.0,15,8000
SH3,0.45,3.00,0.00,0.00,6.20,N/A,1600,9999,1400,2.4,40,8000
SH4,0.85,1.15,0.20,0.00,2.55,N/A,2000,1800,1600,3.0,30,8000
SH5,3.60,2.10,0.00,0.00,2.90,N/A,750,9999,1600,6.0,15,8000
SH6,2.90,1.45,0.00,0.00,1.40,N/A,750,9999,1600,2.0,30,8000
SH7,3.50,5.30,2.20,0.00,3.40,N/A,750,9999,1600,6.0,15,8000
SH8,2.05,3.40,0.85,0.00,4.35,N/A,750,9999,1600,3.0,40,8000
SH9,4.50,2.45,0.00,1.55,7.00,D,750,1800,1500,4.4,40,8000
TU1,0.20,0.90,1.50,0.20,0.90,D,2000,1800,1600,0.6,20,8000
TU2,0.95,1.80,1.25,0.00,0.20,N/A,2000,9999,1600,1.0,30,8000
TU3,1.10,0.15,0.25,0.65,1.10,D,1800,1600,1400,1.3,30,8000
TU4,4.50,0.00,0.00,0.00,2.00,N/A,2300,9999,2000,0.5,12,8000
TU5,4.00,4.00,3.00,0.00,3.00,N/A,1500,9999,750,1.0,25,8000
TL1,1.00,2.20,3.60,0.00,0.00,N/A,2000,9999,9999,0.2,30,8000
TL2,1.40,2.30,2.20,0.00,0.00,N/A,2000,9999,9999,0.2,25,8000
TL3,0.50,2.20,2.80,0.00,0.00,N/A,2000,9999,9999,0.3,20,8000
TL4,0.50,1.50,4.20,0.00,0.00,N/A,2000,9999,9999,0.4,25,8000
TLS,1.15,2.50,4.40,0.00,0.00,N/A,2000,9999,1600,0.6,25,8000
TL6,2.40,1.20,1.20,0.00,0.00,N/A,2000,9999,9999,0.3,25,8000
TL7,0.30,1.40,8.10,0.00,0.00,N/A,2000,9999,9999,0.4,25,8000
TL8,5.80,1.40,1.10,0.00,0.00,N/A,1800,9999,9999,0.3,35,8000
TLO,6.65,3.30,4.15,0.00,0.00,N/A,1800,9999,1600,0.6,35,8000
SB1,1.50,3.00,11.00,0.00,0.00,N/A,2000,9999,9999,1.0,25,8000
SB2,4.50,4.25,4.00,0.00,0.00,N/A,2000,9999,9999,1.0,25,8000
SB3,5.50,2.75,3.00,0.00,0.00,N/A,2000,9999,9999,1.2,25,8000
SB4,5.25,3.50,5.25,0.00,0.00,N/A,2000,9999,9999,2.7,25,8000 """


fpi_fuel_parameters_csv = """
Code,Fuel_Name,windrf,fueldepthm,savr,fuelmce,fueldens,st,se,weight,fci_d,fct,ichap,fgi_1h,fgi_10h,fgi_100h,fgi_1000h,fgi_live,fgi_lh,fgi
1,1: Short grass (1 ft),0.36,0.305,3500,0.12,32,0.0555,0.010,7,0,60,0,0.74,0.00,0.00,0.0,0.00,0.00,0.166
2,2: Timber (grass and understory),0.36,0.305,2784,0.15,32,0.0555,0.010,7,0,60,0,2.00,1.00,0.00,0.0,0.00,0.00,0.896
3,3: Tall grass (2.5 ft),0.44,0.762,1500,0.25,32,0.0555,0.010,7,0,60,0,3.01,0.00,0.00,0.0,0.00,0.00,0.674
4,4: Chaparral (6 ft),0.55,1.829,1739,0.20,32,0.0555,0.010,180,1.123,60,1,5.01,4.01,2.00,0.0,5.01,0.00,3.591
5,5: Brush (2 ft),0.42,0.61,1683,0.20,32,0.0555,0.010,100,0,60,0,1.00,0.50,0.00,0.0,2.00,0.00,0.784
6,6: Dormant brush, hardwood slash,0.44,0.762,1564,0.25,32,0.0555,0.010,100,0,60,0,1.50,2.50,2.00,0.0,0.00,0.00,1.344
7,7: Southern rough,0.44,0.762,1562,0.25,32,0.0555,0.010,100,0,60,0,1.13,1.87,1.50,0.0,0.00,0.00,1.091
8,8: Closed timber litter,0.36,0.0610,1889,0.40,32,0.0555,0.010,100,0,60,0,1.50,1.00,2.50,0.0,0.00,0.00,1.120
9,9: Hardwood litter,0.36,0.0610,2484,0.25,32,0.0555,0.010,100,0,60,0,2.92,0.41,0.15,0.0,0.00,0.00,0.780
10,10: Timber (litter + understory),0.36,0.305,1764,0.20,32,0.0555,0.010,100,0,60,0,3.01,2.00,5.01,0.0,5.01,0.00,3.591
11,11: Light logging slash,0.36,0.305,1182,0.20,32,0.0555,0.010,100,0,60,0,1.50,4.51,5.01,0.0,7.01,0.00,7.749
12,12: Medium logging slash,0.36,0.701,1145,0.20,32,0.0555,0.010,100,0,60,0,4.01,14.03,16.53,0.0,0.00,0.00,13.024
13,13: Heavy logging slash,0.36,0.914,1159,0.25,32,0.0555,0.010,100,0,60,0,7.01,23.04,28.05,0.0,0.00,0.00,1.e-7
14,14: no fuel,0.43,0.305,3500,0.12,32,0.0555,0.010,7,0,60,0,0.00,0.00,0.00,0.0,0.00,0.00,0.000
15,15: Short, Sparse Dry Climate Grass (Dynamic) [GR1 (101)],0.46,0.1219,2200,0.15,32,0.0555,0.010,7,0,60,0,0.10,0.00,0.00,0.0,0.00,0.00,0.0224
16,16: Low Load, Dry Climate Grass (Dynamic) GR2 (102),0.36,0.3048,2000,0.15,32,0.0555,0.010,7,0,60,0,0.10,0.00,0.00,0.0,0.00,0.00,0.0224
17,17: Low Load, Very Coarse, Humid Climate Grass (Dynamic) [GR3 (103)],0.44,0.6096,1500,0.25,32,0.0555,0.010,7,0,60,0,0.10,0.40,0.00,0.0,0.00,0.00,0.1121
18,18: Moderate Load, Dry Climate Grass (Dynamic) [GR4 (104)],0.55,0.6096,1800,0.20,32,0.0555,0.010,9999,0,60,1,5.01,4.01,2.00,0.0,5.01,0.00,3.591
19,19: Low Load, Humid Climate Grass (Dynamic) [GR5 (105)],0.42,0.4572,1683,0.20,32,0.0555,0.010,100,0,60,0,1.00,0.50,0.00,0.0,2.00,0.00,0.784
20,20: Moderate Load, Humid Climate Grass (Dynamic) [GR6 (106)],0.44,0.4572,1564,0.25,32,0.0555,0.010,100,0,60,0,1.50,2.50,2.00,0.0,0.00,0.00,1.344
21,21: High Load, Dry Climate Grass (Dynamic) [GR7 (107)],0.44,0.9144,1562,0.25,32,0.0555,0.010,100,0,60,0,1.13,1.87,1.50,0.0,0.00,0.00,1.091
22,22: High Load, Very Coarse, Humid Climate Grass (Dynamic) [GR8 (108)],0.36,1.2192,9999,0.00,32,0.0555,0.010,9999,0,60,0,0.00,0.00,0.00,0.0,0.00,0.00,0.000
23,23: Very High Load, Humid Climate Grass (Dynamic) [GR9 (109)],0.36,1.5240,9999,0.00,32,0.0555,0.010,9999,0,60,0,0.00,0.00,0.00,0.0,0.00,0.00,0.000
24,24: Low Load, Dry Climate Grass-Shrub (Dynamic) [GS1 (121)],0.36,0.2743,2000,0.15,32,0.0555,0.010,9999,0,60,0,0.20,0.50,0.00,0.0,0.00,0.00,0.000
25,25: Moderate Load, Dry Climate Grass-Shrub (Dynamic) [GS2 (122)],0.36,0.4572,2000,0.25,32,0.0555,0.010,9999,0,60,0,0.30,0.25,0.00,0.0,0.00,0.00,0.000
26,26: Moderate Load, Humid Climate Grass-Shrub (Dynamic) [GS3 (123)],0.36,0.5486,1800,0.15,32,0.0555,0.010,9999,0,60,0,0.30,0.25,0.00,0.0,0.00,0.00,0.000
27,27: High Load, Humid Climate Grass-Shrub (Dynamic) [GS4 (124)],0.36,0.6401,1800,0.25,32,0.0555,0.010,9999,0,60,0,1.90,1.15,0.00,0.0,0.00,0.00,0.000
28,28: Low Load Dry Climate Shrub (Dynamic) [SH1 (141)],0.36,0.3048,9999,0.15,32,0.0555,0.010,9999,0,60,0,0.25,0.00,0.00,0.0,0.00,0.00,0.000
29,29: Moderate Load Dry Climate Shrub [SH2 (142)],0.36,0.3962,9999,0.25,32,0.0555,0.010,9999,0,60,0,0.25,0.00,0.00,0.0,0.00,0.00,0.000
30,30: Moderate Load, Humid Climate Shrub [SH3 (143)],0.36,0.1524,9999,0.25,32,0.0555,0.010,9999,0,60,0,0.00,0.00,0.00,0.0,0.00,0.00,0.000
31,31: Low Load, Humid Climate Timber-Shrub [SH4 (144)],0.36,0.3048,9999,0.15,32,0.0555,0.010,9999,0,60,0,0.85,0.00,0.20,0.0,0.00,0.00,0.000
32,32: High Load, Dry Climate Shrub [SH5 (145)],0.36,0.3048,9999,0.40,32,0.0555,0.010,9999,0,60,0,3.60,0.00,0.00,0.0,0.00,0.00,0.000
33,33: Low Load, Humid Climate Shrub [SH6 (146)],0.36,0.3962,9999,0.30,32,0.0555,0.010,9999,0,60,0,2.90,0.00,1.40,0.0,0.00,0.00,0.000
34,34: Very High Load, Dry Climate Shrub [SH7 (147)],0.36,0.1524,9999,0.25,32,0.0555,0.010,9999,0,60,0,3.50,0.00,0.00,0.0,0.00,0.00,0.000
35,35: High Load, Humid Climate Shrub [SH8 (148)],0.36,0.3048,9999,0.15,32,0.0555,0.010,9999,0,60,0,2.05,0.00,0.85,0.0,0.00,0.00,0.000
36,36: Very High Load, Humid Climate Shrub (Dynamic) [SH9 (149)],0.36,0.3048,1500,0.40,32,0.0555,0.010,9999,0,60,0,4.50,0.00,0.00,0.0,0.00,0.00,0.000
37,37: Low Load Dry Climate Timber-Grass-Shrub (Dynamic) [TU1 (161)],0.36,0.3048,9999,0.20,32,0.0555,0.010,9999,0,60,0,0.20,0.00,1.00,0.0,0.00,0.00,0.5828
38,38: Moderate Load, Humid Climate Timber-Shrub [TU2 (162)],0.36,0.3962,9999,0.30,32,0.0555,0.010,9999,0,60,0,0.95,1.80,1.25,0.0,0.00,0.00,0.8967
39,39: Moderate Load, Humid Climate Timber-Grass-Shrub (Dynamic) [TU3 (163)],0.36,0.1524,9999,0.20,32,0.0555,0.010,9999,0,60,0,1.10,0.15,0.25,0.0,0.00,0.00,0.3363
40,40: Dwarf Conifer With Understory [TU4 (164)],0.36,0.3658,9999,0.25,32,0.0555,0.010,9999,0,60,0,4.50,0.00,0.00,0.0,0.00,0.00,0.000
41,41: Very High Load, Dry Climate Timber-Shrub [TU5 (165)],0.36,0.8230,9999,0.25,32,0.0555,0.010,9999,0,60,0,4.00,4.00,3.00,0.0,0.00,0.00,0.000
42,42: Low Load Compact Conifer Litter [TL1 (181)],0.36,0.3658,9999,0.12,32,0.0555,0.010,9999,0,60,0,1.00,2.20,3.60,0.0,0.00,0.00,0.000
43,43: Low Load Broadleaf Litter [TL2 (182)],0.36,0.3048,9999,0.25,32,0.0555,0.010,9999,0,60,0,2.20,2.30,2.20,0.0,0.00,0.00,0.000
44,44: Moderate Load Conifer Litter [TL3 (183)],0.36,0.5486,9999,0.25,32,0.0555,0.010,9999,0,60,0,2.80,2.20,2.80,0.0,0.00,0.00,0.000
45,45: Small downed logs [TL4 (184)],0.36,0.6401,9999,0.25,32,0.0555,0.010,9999,0,60,0,4.20,4.20,4.20,0.0,0.00,0.00,0.000
46,46: High Load Conifer Litter [TL5 (185)],0.36,0.3658,9999,0.25,32,0.0555,0.010,9999,0,60,0,1.20,1.20,1.20,0.0,0.00,0.00,0.000
47,47: Moderate Load Broadleaf Litter [TL6 (186)],0.36,0.3048,9999,0.25,32,0.0555,0.010,9999,0,60,0,1.40,1.40,1.40,0.0,0.00,0.00,0.000
48,48: Large Downed Logs [TL7 (187)],0.36,0.3658,9999,0.25,32,0.0555,0.010,9999,0,60,0,8.10,1.10,1.10,0.0,0.00,0.00,0.000
49,49: Long-Needle Litter [TL8 (188)],0.36,0.3658,9999,0.25,32,0.0555,0.010,9999,0,60,0,1.10,1.10,4.15,0.0,0.00,0.00,0.000
50,50: Very High Load Broadleaf Litter [TL9 (189)],0.36,0.8230,9999,0.25,32,0.0555,0.010,9999,0,60,0,6.65,0.00,0.00,0.0,0.00,0.00,0.000
51,51: Low Load Activity Fuel [SB1 (201)],0.36,0.3048,9999,0.25,32,0.0555,0.010,9999,0,60,0,1.50,0.00,0.00,0.0,0.00,0.00,0.000
52,52: Moderate Load Activity Fuel or Low Load Blowdown [SB2 (202)],0.36,0.3048,9999,0.25,32,0.0555,0.010,9999,0,60,0,4.50,0.00,0.00,0.0,0.00,0.00,0.000
53,53: High Load Activity Fuel or Moderate Load Blowdown [SB3 (203)],0.36,0.3962,9999,0.25,32,0.0555,0.010,9999,0,60,0,5.50,0.00,0.00,0.0,0.00,0.00,0.000
54,54: High Load Blowdown [SB4 (204)],0.36,0.1524,9999,0.25,32,0.0555,0.010,9999,0,60,0,5.25,0.00,0.00,0.0,0.00,0.00,0.000
"""


import csv
import io
from .model_set import *


def to_csv(dicts):
    if not dicts:
        return "No data to convert."

    # Extract the keys from the first dictionary as headers
    headers = list(dicts[0].keys())

    # Create a string buffer
    csv_string = io.StringIO()

    # Use DictWriter to write data to the string buffer
    writer = csv.DictWriter(csv_string, fieldnames=headers)

    # Write the header
    writer.writeheader()

    # Write the data
    for d in dicts:
        writer.writerow(d)

    # Get the string from the buffer
    csv_content = csv_string.getvalue()
    csv_string.close()

    return csv_content


def load_csv(csv_string):
    def convert_if_number(s):
        try:
            return int(s) if "." not in s else float(s)
        except ValueError:
            return s

    def remove_suffix_for_check(key):
        return key.split("_")[0]

    # Split the CSV string into lines
    lines = csv_string.strip().split("\n")

    # Extract the original headers
    original_headers = lines[0].split(",")

    # Check if headers (without suffix) are in fuel_properties
    for header in original_headers:
        check_header = remove_suffix_for_check(header)
        if check_header not in var_properties:
            print(
                f"Warning: '{check_header}' not found in as typical fuel parameter name"
            )

    # List to store all the dictionaries
    dict_list = []

    # Iterate over each line except the header
    for line in lines[1:]:
        values = [convert_if_number(value) for value in line.split(",")]
        dict_entry = {
            header: value for header, value in zip(original_headers, values)
        }  # if remove_suffix_for_check(header) in var_properties}
        dict_list.append(model_parameters(dict_entry))

    return dict_list


def to_latex(data_dict):
    """
    Generate a LaTeX table from a fuel dictionary.

    Parameters:
    - data_dict (dict): A dictionary with the fuel data.

    Returns:
    - str: A string containing the LaTeX code for the table.
    """
    # Start the LaTeX table code with the longtable environment
    first_row_values = next(iter(data_dict.values()))
    num_columns = len(first_row_values) + 1  # Additional column for the code
    latex_table = (
        "\\begin{longtable}{|" + "l|" + "c|" * (num_columns - 1) + "}\n\\hline\n"
    )

    # Generate column headers from the keys of the first item in the dictionary and escape underscores for LaTeX
    headers = first_row_values.keys()
    escaped_headers = ["Code"] + [key.replace("_", " ") for key in headers]
    latex_table += (
        " & ".join(escaped_headers) + " \\\\\n\\hline\n\\endfirsthead\n\\hline\n"
    )

    # Add the data rows
    for code, values in data_dict.items():
        row_data = [str(values[key]) for key in headers]
        escaped_row_data = [data.replace("_", "\\_") for data in row_data]
        latex_table += code + " & " + " & ".join(escaped_row_data) + " \\\\\n\\hline\n"

    # End the LaTeX table code
    latex_table += "\\end{longtable}"

    return latex_table


def to_latex_file(table_dict, latex_file_name):
    """
    Create a complete LaTeX document that can be compiled directly, containing the given table.

    Parameters:
    - table_dict (str):  A dictionary with the fuel data.
    - latex_file_name (str) : File path of latex output file

    Returns:
    - Nothing
    """
    # LaTeX document structure

    latex_document = (
        "\\documentclass{article}\n"
        "\\usepackage[utf8]{inputenc}\n"
        "\\usepackage{longtable}\n"
        "\\usepackage{lscape}\n"  # Package for landscape orientation
        "\\begin{document}\n"
        "\\begin{landscape}\n"
        "\\tiny\n"  # Set the font size to small
        + to_latex_table(table_dict)
        + "\n\\end{landscape}\n"
        "\\end{document}"
    )

    with open(latex_file_name, "w") as file:
        file.write(latex_document)
