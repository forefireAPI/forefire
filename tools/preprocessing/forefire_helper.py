import numpy as np
import matplotlib.pyplot as plt
import matplotlib.path as mpath
import matplotlib.patches as mpatches
import matplotlib.colors as mcolors
import math
import xarray as xr
import pyforefire as forefire
from datetime import datetime
import time

def standardRothermelFuelTable():
    return """Index;Rhod;Rhol;Md;Ml;sd;sl;e;Sigmad;Sigmal;stoch;RhoA;Ta;Tau0;Deltah;DeltaH;Cp;Cpa;Ti;X0;r00;Blai;me
111;500.0;500.0;0.15;0.5;2400.0;5700.0;0;0.0;0;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
112;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
121;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
122;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
123;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
124;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
131;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
132;720.0;720.0;0.08;1;5544.0;4766.0;0;0.89;1.79;8.3;1.0;300.0;75590.0;2300000.0;1.6E7;2000.0;1100.0;600.0;0.3;2.5E-5;4.0;0.3
133;720.0;720.0;0.08;1;5544.0;4766.0;0;0.89;1.79;8.3;1.0;300.0;75590.0;2300000.0;1.6E7;2000.0;1100.0;600.0;0.3;2.5E-5;4.0;0.3
141;512.0;512.0;0.08;1;6562;5905;0.46;0.22;0.25;8.3;1.0;300.0;75590.0;2300000.0;1.86E7;1800.0;1000.0;600.0;0.3;2.5E-05;4.0;0.3
142;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
211;500.0;500.0;0.13;0.5;2400.0;5700.0;1;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
212;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
213;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
221;500.0;500.0;0.13;0.5;2400.0;5700.0;0.5;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
222;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
223;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
231;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
241;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
242;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
243;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
244;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
311;512.0;512.0;0.13;0.5;4922.0;2460.0;0.3;0.9;0.67;8.3;1.0;300.0;70000.0;2300000.0;1.86E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
312;512.0;512.0;0.13;0.5;4922.0;2460.0;0.3;0.9;0.67;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
313;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
321;512.0;512.0;0.13;0.5;5905.0;5250.0;0.46;0.54;0.11;8.3;1.0;300.0;70000.0;2300000.0;1.86E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
322;512.0;512.0;0.13;0.5;2460.0;5250.0;1.2;0.8;0.65;8.3;1.0;300.0;70000.0;2300000.0;1.86E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
323;512.0;512.0;0.13;0.5;2460.0;5250.0;1.8;0.8;0.65;8.3;1.0;300.0;70000.0;2300000.0;1.86E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
324;512.0;512.0;0.13;0.5;2460.0;5250.0;1.8;0.8;0.65;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
331;500.0;500.0;1.6;2;2400.0;5700.0;0;10;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
332;500.0;500.0;10;10;2400.0;5700.0;0;10;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
333;512.0;512.0;0.08;0.5;6560.0;5900.0;0.3;0.2;0.05;8.3;1.0;300.0;70000.0;2300000.0;1.86E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
334;512.0;512.0;0.08;0.5;6560.0;5900.0;0.3;0.2;0.05;8.3;1.0;300.0;70000.0;2300000.0;1.86E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
335;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
411;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
412;500.0;500.0;0.13;0.5;2400.0;5700.0;0.1;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
421;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
422;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
423;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
511;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
512;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
521;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
522;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
523;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3"""


def extendedRothermelFuelTable():
    return """Index;Rhod;Rhol;Md;Ml;sd;sl;e;Sigmad;Sigmal;stoch;RhoA;Ta;Tau0;Deltah;DeltaH;Cp;Cpa;Ti;X0;r00;Blai;me
0;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
1;563.0;522.0;0.1;1.0;6099.0;7273.0;0.24;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
2;614.0;613.0;0.1;1.0;4287.0;5738.0;0.4;1.378;0.174;8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
3;614.0;613.0;0.1;1.0;4287.0;5738.0;0.4;1.378;0.174;8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
4;613.0;538.0;0.1;1.0;4357.0;6524.0;0.19;1.286;0.085;8.3;1.0;300;70000;18677000.0;18677000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
5;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
6;562.0;474.0;0.1;1.0;6740.0;8195.0;0.57;1.326;0.166;8.3;1.0;300;70000;18941000.0;18941000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
7;658.0;651.0;0.1;1.0;4734.0;5733.0;0.15;1.415;0.541;8.3;1.0;300;70000;18472000.0;18466000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
8;446.0;513.0;0.1;1.0;7792.0;9072.0;0.78;0.492;0.023;8.3;1.0;300;70000;18587000.0;18587000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
9;467.0;543.0;0.1;1.0;6115.0;7224.0;0.285;0.855;0.174;8.3;1.0;300;70000;18474000.0;18474000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
10;674.0;612.0;0.1;1.0;4801.0;5928.0;0.45;1.525;0.709;8.3;1.0;300;70000;18280000.0;18277000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
11;653.0;582.0;0.1;1.0;4753.0;6569.0;0.75;1.096;1.105;8.3;1.0;300;70000;18226000.0;18221000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
12;596.0;586.0;0.1;1.0;3688.0;5551.0;0.475;1.346;0.077;8.3;1.0;300;70000;19050000.0;19050000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
13;438.0;488.0;0.1;1.0;7274.0;8453.0;0.38;1.053;0.321;8.3;1.0;300;70000;17842000.0;17842000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
14;634.0;570.0;0.1;1.0;5518.0;7704.0;0.2;1.293;0.402;8.3;1.0;300;70000;18507000.0;18507000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
15;667.0;636.0;0.1;1.0;5162.0;7145.0;0.3;1.457;0.519;8.3;1.0;300;70000;18851000.0;18851000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
16;563.0;531.0;0.1;1.0;4694.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19235000.0;19235000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
17;599.0;531.0;0.1;1.0;5813.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19041000.0;19041000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
18;565.0;531.0;0.1;1.0;4644.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19169000.0;19169000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
19;565.0;531.0;0.1;1.0;4644.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19169000.0;19169000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
20;678.0;573.0;0.1;1.0;6064.0;5456.0;0.475;1.604;0.241;8.3;1.0;300;70000;18992000.0;18992000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
21;597.0;442.0;0.1;1.0;7975.0;10000.0;0.09;0.566;0.008;8.3;1.0;300;70000;18887000.0;18887000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
22;551.0;545.0;0.1;1.0;5360.0;7065.0;1.0;1.283;0.261;8.3;1.0;300;70000;19021000.0;19021000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
23;565.0;531.0;0.1;1.0;4644.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19169000.0;19169000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
24;659.0;609.0;0.1;1.0;6691.0;6974.0;0.2;0.923;0.469;8.3;1.0;300;70000;18920000.0;18920000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
25;491.0;482.0;0.1;1.0;5732.0;7599.0;0.77;1.287;0.247;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
26;597.0;442.0;0.1;1.0;7975.0;10000.0;0.09;0.566;0.008;8.3;1.0;300;70000;18887000.0;18887000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
27;590.0;471.0;0.1;1.0;7687.0;8345.0;0.475;0.259;0.139;8.3;1.0;300;70000;18715000.0;18715000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
28;565.0;491.0;0.1;1.0;4522.0;6577.0;0.8;1.343;0.125;8.3;1.0;300;70000;18866000.0;18864000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
29;559.0;481.0;0.1;1.0;4488.0;7793.0;0.38;1.139;0.106;8.3;1.0;300;70000;19207000.0;19207000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
30;533.0;527.0;0.1;1.0;3886.0;4358.0;0.42;1.0;0.251;8.3;1.0;300;70000;18508000.0;18499000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
31;625.0;601.0;0.1;1.0;5210.0;5747.0;0.475;1.188;1.006;8.3;1.0;300;70000;18922000.0;18919000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
32;625.0;601.0;0.1;1.0;5210.0;5747.0;0.475;1.188;1.006;8.3;1.0;300;70000;18922000.0;18919000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
33;500.0;442.0;0.1;1.0;8359.0;10000.0;0.27;0.469;0.0;8.3;1.0;300;70000;17129000.0;17129000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
34;621.0;600.0;0.1;1.0;5314.0;5947.0;0.665;1.103;0.811;8.3;1.0;300;70000;18853000.0;18847000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
35;502.0;442.0;0.1;1.0;8135.0;10000.0;0.3;0.482;0.0;8.3;1.0;300;70000;16931000.0;16931000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
36;592.0;547.0;0.1;1.0;5819.0;7229.0;0.57;1.209;0.606;8.3;1.0;300;70000;18426000.0;18422000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
37;502.0;442.0;0.1;1.0;8888.0;10000.0;0.3;0.482;0.0;8.3;1.0;300;70000;17103000.0;17103000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
38;624.0;603.0;0.1;1.0;5341.0;5776.0;1.2;1.533;1.387;8.3;1.0;300;70000;18912000.0;18907000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
39;618.0;584.0;0.1;1.0;6104.0;6996.0;0.76;0.549;0.499;8.3;1.0;300;70000;19075000.0;19029000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
40;618.0;584.0;0.1;1.0;6104.0;6996.0;0.76;0.549;0.499;8.3;1.0;300;70000;19075000.0;19029000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
41;618.0;584.0;0.1;1.0;6104.0;6996.0;0.76;0.549;0.499;8.3;1.0;300;70000;19075000.0;19029000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
42;762.0;761.0;0.1;1.0;6216.0;6985.0;0.728;0.809;1.069;8.3;1.0;300;70000;19130000.0;18581000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
43;644.0;595.0;0.1;1.0;6667.0;6781.0;0.95;0.426;0.624;8.3;1.0;300;70000;19036000.0;18986000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
44;577.0;550.0;0.1;1.0;5307.0;6592.0;0.665;0.836;0.344;8.3;1.0;300;70000;19046000.0;19021000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
45;577.0;550.0;0.1;1.0;5307.0;6592.0;0.665;0.217;0.344;8.3;1.0;300;70000;19046000.0;19021000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
46;544.0;486.0;0.1;1.0;5101.0;7518.0;0.57;0.109;0.159;8.3;1.0;300;70000;19295000.0;19295000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
47;623.0;651.0;0.1;1.0;4102.0;5421.0;0.2;1.349;0.12;8.3;1.0;300;70000;18632000.0;18632000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
48;615.0;606.0;0.1;1.0;4545.0;4814.0;0.6;1.009;1.126;8.3;1.0;300;70000;18771000.0;18760000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
49;591.0;584.0;0.1;1.0;4441.0;4857.0;1.05;1.576;0.818;8.3;1.0;300;70000;19322000.0;19322000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
50;661.0;617.0;0.1;1.0;6252.0;7700.0;0.18;0.321;0.647;8.3;1.0;300;70000;17630000.0;17529000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
51;831.0;792.0;0.1;1.0;6758.0;7320.0;0.2;0.407;1.134;8.3;1.0;300;70000;19144000.0;18486000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
52;442.0;442.0;0.1;1.0;10000.0;10000.0;0.3;0.04;0.092;8.3;1.0;300;70000;16298000.0;16298000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
53;554.0;442.0;0.1;1.0;7236.0;10000.0;0.21;0.128;0.065;8.3;1.0;300;70000;17428000.0;17428000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
54;436.0;464.0;0.1;1.0;6435.0;8759.0;0.285;0.78;0.102;8.3;1.0;300;70000;18396000.0;18396000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
55;554.0;442.0;0.1;1.0;7236.0;10000.0;0.14;0.118;0.043;8.3;1.0;300;70000;17428000.0;17428000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
62;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
75;614.0;613.0;0.25;1.0;4287.0;5738.0;0.1;1.378;0.174;8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
73;613.0;538.0;0.1;1.0;4357.0;6524.0;0.19;1.286;0.085;8.3;1.0;300;70000;18677000.0;18677000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
82;614.0;613.0;0.1;1.0;4287.0;5738.0;0.4;1.378;0.174;8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
83;563.0;522.0;0.05;1.0;6099.0;7273.0;0.4;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
102;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
103;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
104;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
105;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
106;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
111;500.0;500.0;0.15;0.5;2400.0;5700.0;0;0.0;0;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
112;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
121;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
123;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
124;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
131;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
132;720.0;720.0;0.08;1;5544.0;4766.0;0;0.89;1.79;8.3;1.0;300.0;75590.0;2300000.0;1.6E7;2000.0;1100.0;600.0;0.3;2.5E-5;4.0;0.3
133;720.0;720.0;0.08;1;5544.0;4766.0;0;0.89;1.79;8.3;1.0;300.0;75590.0;2300000.0;1.6E7;2000.0;1100.0;600.0;0.3;2.5E-5;4.0;0.3
141;720.0;720.0;0.08;1;5544.0;4766.0;0.5;0.89;1.79;8.3;1.0;300.0;75590.0;2300000.0;1.6E7;2000.0;1100.0;600.0;0.3;2.5E-5;4.0;0.3
142;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
162;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
211;500.0;500.0;0.13;0.5;2400.0;5700.0;1;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
212;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
213;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
221;500.0;500.0;0.13;0.5;2400.0;5700.0;0.5;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
222;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
223;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
231;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
241;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
242;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
243;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
244;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
255;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
311;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
312;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
313;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
321;500.0;500.0;0.13;0.5;2400.0;5700.0;0.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
322;500.0;500.0;0.13;0.5;2400.0;5700.0;2.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
323;500.0;500.0;0.08;0.4;6000.0;5000.0;2;0.4;1.8;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
324;500.0;500.0;0.14;0.5;2400.0;5700.0;1.6;10;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
331;500.0;500.0;1.6;2;2400.0;5700.0;0;10;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
332;500.0;500.0;10;10;2400.0;5700.0;0;10;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
333;500.0;500.0;2.13;2.5;2400.0;5700.0;0.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
334;500.0;500.0;2.13;2.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
335;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
411;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
412;500.0;500.0;0.13;0.5;2400.0;5700.0;0.1;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
421;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
422;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
423;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
511;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
512;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
521;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
522;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
523;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3"""


def testAnnFuelTable():
    return """Index;Rhod;sd;Ta
0;0.24;0.68;0.097
1;0.95;0.73;0.44
2;0.82;0.36;0.050
3;0.77;0.11;0.05
4;0.50;0.92;0.22"""

def genAltitudeMap(slope_coef, sub_sim_shape, data_resolution):
    """
    Generate a matrix of altitudes given a slope coefficient
    """
    slope = np.linspace(0, 1, sub_sim_shape[1])
    slope = np.repeat(slope, sub_sim_shape[0])
    slope = slope.reshape(sub_sim_shape[1], sub_sim_shape[0]).T
    return slope * slope_coef * (data_resolution / 5)

#  Functions definitions

def get_multi_sub_domain_indices_from_location(x, y, originX, originY, domain_width, domain_height, shape_multisim):
    """
    Used for retrieve indices of coordinates inside simulation matrix
    """
    i = np.floor(((x - originX) / domain_width) * shape_multisim[0])
    j = np.floor(((y - originY) / domain_height) * shape_multisim[1])
    return int(i), int(j)

def get_sub_domain_indices_from_location(x, y, originX, originY, domain_width, domain_height):
    """
    Used for retrieve indices of coordinates inside simulation matrix
    """
    i = np.floor(((x - originX) / domain_width))
    j = np.floor(((y - originY) / domain_height))
    return int(i), int(j)

def maxDiff(a):
    """
    Used for get the maximum difference along first axis of an array
    """
    vmin = a[0]
    dmax = 0
    for i in range(len(a)):
        if (a[i] < vmin):
            vmin = a[i]
        elif (a[i] - vmin > dmax):
            dmax = a[i] - vmin
    return dmax

def getLocationFromLine(line):
    """
    Return the location of current node (line).
    """
    llv = line.split("loc=(")
    if len(llv) < 2: 
        return None
    llr = llv[1].split(",")
    if len(llr) < 3: 
        return None
    return (float(llr[0]),float(llr[1]))

def printToPathe(linePrinted):
    """
    Compute the current results of simulation to pathes.
    """
    fronts = linePrinted.split("FireFront")
    pathes = []
    for front in fronts[1:]:
        nodes = front.split("FireNode")[1:]
        if len(nodes) > 0:
            Path = mpath.Path
            codes = []
            verts = []
            firstNode = getLocationFromLine(nodes[0])
            codes.append(Path.MOVETO)
            verts.append(firstNode)

            for node in nodes[:]:
                newNode = getLocationFromLine(node)
                codes.append(Path.LINETO)
                verts.append(newNode)
            codes.append(Path.LINETO)
            verts.append(firstNode)
            pathes.append(mpath.Path(verts, codes))

    return pathes



import struct
import zlib

def write_png_header(width, height):
    # PNG file signature
    png_signature = b'\x89PNG\r\n\x1a\n'
    
    # IHDR chunk: width, height, bit depth, color type, compression, filter, interlace
    # Color type 4: grayscale with alpha, Bit depth 8
    ihdr_data = struct.pack(">IIBBBBB", width, height, 8, 4, 0, 0, 0)
    return png_signature + create_chunk(b'IHDR', ihdr_data)

def create_chunk(chunk_type, data):
    # Chunk structure: length, type, data, CRC
    chunk_length = struct.pack(">I", len(data))
    chunk_type = chunk_type
    chunk_crc = struct.pack(">I", zlib.crc32(chunk_type + data) & 0xffffffff)
    return chunk_length + chunk_type + data + chunk_crc

def write_png_data(data, width, height):
    # Modify to handle transparency for grayscale value 255
    raw_data = b''
    for y in range(height):
        row_data = b'\x00'  # Filter type 0 (None)
        for x in range(width):
            gray = data[y * width + x]
            alpha = 0 if gray == 255 else 255  # Transparent if gray is 255
            row_data += struct.pack("BB", gray, alpha)  # Grayscale value followed by alpha value
        raw_data += row_data
    compressed_data = zlib.compress(raw_data)  # Compress the data as required by the PNG specification
    return create_chunk(b'IDAT', compressed_data)

def write_png_file(filename, data, width, height):
    with open(filename, 'wb') as f:
        # Write header
        f.write(write_png_header(width, height))
        
        # Write image data
        f.write(write_png_data(data, width, height))
        
        # Write IEND chunk
        f.write(create_chunk(b'IEND', b''))


from matplotlib.colors import ListedColormap

def map_fuel_to_colors(fuelmap, fuel_list):
    """
    Convert a fuel_map for use with colors.
    The returned fuel_map values are replaced by indices of fuels from fuel_list.
    """
    for i in range(len(fuelmap)):
        for j in range(len(fuelmap[0])):
            try:
                fuelmap[i][j] = fuel_list.index(fuelmap[i][j]) + 1
            except ValueError:
                fuelmap[i][j] = 0
    return fuelmap

def fill_random(s, k, value_yes, value_no=0):
    """Generate a randomly filled array."""
    a = np.random.random(size=s)
    return np.where(a > k, value_yes, value_no)


def computeSpeed(atime):
    """
    Computes the speed as the inverse of the gradient of arrival times.
    'inf' in the arrival times indicates that the point was never reached.

    Parameters:
        atime (np.array): 2D array of arrival times.

    Returns:
        np.array: 2D array of speeds, with the same shape as atime.
                  Returns 'inf' where the arrival time is 'inf', indicating no arrival.
    """
    # Check where the times are infinite
    inf_mask = np.isinf(atime)

    # Replace 'inf' with the maximum finite value in atime
    max_finite = np.nanmax(np.where(inf_mask, np.nan, atime))
    atime_temp = np.where(inf_mask, max_finite, atime)

    # Compute the gradient in the x and y directions on the modified array
    grad_y, grad_x = np.gradient(atime_temp)
    
    # Compute the magnitude of the gradient
    grad_mag = np.sqrt(grad_x**2 + grad_y**2)

    # Speed is the inverse of the gradient magnitude
    # Avoid division by zero by adding a small number in the denominator
    speed = np.where(grad_mag == 0, np.inf, 1 / (grad_mag + 1e-10))

    # Re-assign 'inf' to the speed array where the original arrival time was 'inf'
    speed[inf_mask] = np.inf

    return speed


def plot_simulation(pathes, fuel_map, elevation_map, myExtents, scalMap = None):
    """
    Used for plot 4 axis graph, with Heatflux, Fuels, Altitude plotted under simulation, 
    and Statistics for the last axis.
    """
    # Create a figure with 2 axis (2 subplots)
    fig, ax = plt.subplots(figsize=(10,7), dpi=120)

    # Get fuel_map matrix
    if fuel_map is not None:
        fuels = fuel_map
        
        fuel_types = { # ESA EU and COrine
            10: {'color': (0, 100, 0), 'description': 'Tree cover'},
            20: {'color': (255, 187, 34), 'description': 'Shrubland'},
            30: {'color': (255, 255, 76), 'description': 'Grassland'},
            40: {'color': (240, 150, 255), 'description': 'Cropland'},
            50: {'color': (250, 0, 0), 'description': 'Built-up'},
            60: {'color': (180, 180, 180), 'description': 'Bare / sparse vegetation'},
            70: {'color': (240, 240, 240), 'description': 'Snow and Ice'},
            80: {'color': (0, 100, 200), 'description': 'Permanent water bodies'},
            90: {'color': (0, 150, 160), 'description': 'Herbaceous wetland'},
            95: {'color': (0, 207, 117), 'description': 'Mangroves'},
            100: {'color': (250, 230, 160), 'description': 'Moss and lichen'},
            0: {'color': (255, 255, 255), 'description': 'Clouds'},
            62: {'color': (210, 0, 0),'description': 'Artificial surfaces and constructions'},
            73: {'color': (253, 211, 39), 'description': 'Cultivated areas'},
            75: {'color': (176, 91, 16), 'description': 'Vineyards'},
            82: {'color': (35, 152, 0), 'description': 'Broadleaf tree cover'},
            83: {'color': (8, 98, 0), 'description': 'Coniferous tree cover'},
            102: {'color': (249, 150, 39), 'description': 'Herbaceous vegetation'},
            103: {'color': (141, 139, 0), 'description': 'Moors and Heathland'},
            104: {'color': (95, 53, 6), 'description': 'Sclerophyllous vegetation'},
            105: {'color': (149, 107, 196), 'description': 'Marshes'},
            106: {'color': (77, 37, 106), 'description': 'Peatbogs'},
            121: {'color': (154, 154, 154), 'description': 'Natural material surfaces'},
            123: {'color': (106, 255, 255),'description': 'Permanent snow covered surfaces'},
            162: {'color': (20, 69, 249), 'description': 'Water bodies'},
            255: {'color': (255, 255, 255), 'description': 'No data'},
            111: {'color': (230, 0, 77), 'description': 'Urban fabric, Continuous urban fabric'},
            112: {'color': (255, 0, 0), 'description': 'Urban fabric, Discontinuous urban fabric'},
            121: {'color': (204, 77, 242), 'description': 'Industrial, commercial and transport units, Industrial or commercial units'},
            122: {'color': (204, 0, 0), 'description': 'Industrial, commercial and transport units, Road and rail networks and associated land'},
            123: {'color': (230, 204, 204), 'description': 'Industrial, commercial and transport units, Port areas'},
            124: {'color': (230, 204, 230), 'description': 'Industrial, commercial and transport units, Airports'},
            131: {'color': (166, 0, 204), 'description': 'Mine, dump and construction sites, Mineral extraction sites'},
            132: {'color': (166, 77, 0), 'description': 'Mine, dump and construction sites, Dump sites'},
            133: {'color': (255, 77, 255), 'description': 'Mine, dump and construction sites, Construction sites'},
            141: {'color': (255, 166, 255), 'description': 'Artificial, non-agricultural vegetated areas, Green urban areas'},
            142: {'color': (255, 230, 255), 'description': 'Artificial, non-agricultural vegetated areas, Sport and leisure facilities'},
            211: {'color': (255, 255, 168), 'description': 'Agricultural areas, Arable land, Non-irrigated arable land'},
            212: {'color': (255, 255, 0), 'description': 'Agricultural areas, Arable land, Permanently irrigated land'},
            213: {'color': (230, 230, 0), 'description': 'Agricultural areas, Arable land, Rice fields'},
            221: {'color': (230, 128, 0), 'description': 'Agricultural areas, Permanent crops, Vineyards'},
            222: {'color': (242, 166, 77), 'description': 'Agricultural areas, Permanent crops, Fruit trees and berry plantations'},
            223: {'color': (230, 166, 0), 'description': 'Agricultural areas, Permanent crops, Olive groves'},
            231: {'color': (230, 230, 77), 'description': 'Agricultural areas, Pastures'},
            241: {'color': (255, 230, 166), 'description': 'Agricultural areas, Heterogeneous agricultural areas, Annual crops associated with permanent crops'},
            242: {'color': (255, 230, 77), 'description': 'Agricultural areas, Heterogeneous agricultural areas, Complex cultivation patterns'},
            243: {'color': (230, 204, 77), 'description': 'Agricultural areas, Heterogeneous agricultural areas, Land principally occupied by agriculture, with significant areas of natural vegetation'},
            244: {'color': (242, 204, 166), 'description': 'Agricultural areas, Heterogeneous agricultural areas, Agro-forestry areas'},
            311: {'color': (128, 255, 0), 'description': 'Forest and semi natural areas, Forests, Broad-leaved forest'},
            312: {'color': (0, 166, 0), 'description': 'Forest and semi natural areas, Forests, Coniferous forest'},
            313: {'color': (77, 255, 0), 'description': 'Forest and semi natural areas, Forests, Mixed forest'},
            321: {'color': (204, 242, 77), 'description': 'Forest and semi natural areas, Scrub and/or herbaceous vegetation associations, Natural grasslands'},
            322: {'color': (166, 255, 128), 'description': 'Forest and semi natural areas, Scrub and/or herbaceous vegetation associations, Moors and heathland'},
            323: {'color': (166, 230, 77), 'description': 'Forest and semi natural areas, Scrub and/or herbaceous vegetation associations, Sclerophyllous vegetation'},
            324: {'color': (166, 242, 0), 'description': 'Forest and semi natural areas, Scrub and/or herbaceous vegetation associations, Transitional woodland-shrub'},
            331: {'color': (230, 230, 230), 'description': 'Forest and semi natural areas, Open spaces with little or no vegetation, Beaches, dunes, sands'},
            332: {'color': (204, 204, 204), 'description': 'Forest and semi natural areas, Open spaces with little or no vegetation, Bare rocks'},
            333: {'color': (204, 255, 204), 'description': 'Forest and semi natural areas, Open spaces with little or no vegetation, Sparsely vegetated areas'},
            334: {'color': (0, 0, 0), 'description': 'Forest and semi natural areas, Open spaces with little or no vegetation, Burnt areas'},
            335: {'color': (166, 230, 204), 'description': 'Forest and semi natural areas, Open spaces with little or no vegetation, Glaciers and perpetual snow'},
            411: {'color': (166, 166, 255), 'description': 'Wetlands, Inland wetlands, Inland marshes'},
            412: {'color': (77, 77, 255), 'description': 'Wetlands, Inland wetlands, Peat bogs'},
            421: {'color': (204, 204, 255), 'description': 'Wetlands, Maritime wetlands, Salt marshes'},
            422: {'color': (230, 230, 255), 'description': 'Wetlands, Maritime wetlands, Salines'},
            423: {'color': (166, 166, 230), 'description': 'Wetlands, Maritime wetlands, Intertidal flats'},
            511: {'color': (0, 204, 242), 'description': 'Water bodies, Inland waters, Water courses'},
            512: {'color': (128, 242, 230), 'description': 'Water bodies, Inland waters, Water bodies'},
            521: {'color': (0, 255, 166), 'description': 'Water bodies, Marine waters, Coastal lagoons'},
            522: {'color': (166, 255, 230), 'description': 'Water bodies, Marine waters, Estuaries'},
            523: {'color': (230, 242, 255), 'description': 'Water bodies, Marine waters, Sea and ocean'}
            }
        # Normalize the colors to the [0, 1] range expected by matplotlib
        for key in fuel_types:
            rgb = fuel_types[key]['color']
            fuel_types[key]['color'] = tuple([x/255.0 for x in rgb])
        
        # Generate the list of colors
        colors = [fuel_types[key]['color'] for key in sorted(fuel_types.keys())]
        
        # Create the colormap
        lcmap = ListedColormap(colors) 
        
        CS = ax.imshow(fuels, cmap=lcmap, interpolation='nearest', origin='lower', extent=myExtents)
       # norm = mcolors.BoundaryNorm(bounds, cmap.N)
        plt.colorbar(CS)

    
    if elevation_map is not None:
        elevation = elevation_map#np.transpose(elevation_map.reshape(elevation_map.shape[0], elevation_map.shape[1], 1))[0]
        
        contour_levels = np.arange(np.min(elevation), np.max(elevation), 200)  # Contours every 200m
        ax.contour(elevation, levels=contour_levels, colors='white', origin='lower', extent=myExtents, linewidths=0.5, linestyles='solid')
    
    if scalMap is not None:
        CS = ax.imshow(scalMap, origin='lower', extent=myExtents)
        plt.colorbar(CS)

    # bounds = [0, 1]
    # cmap = mpl.colors.ListedColormap(['brown', 'green'])
    # norm = mpl.colors.BoundaryNorm(bounds, cmap.N)
    # CS = ax.imshow(fuels, cmap=cmap, interpolation='nearest', origin='lower', extent=(0,domain_width,0,domain_height))
    # # plt.colorbar(plt.cm.ScalarMappable(cmap=cmap, norm=norm), cax=ax)
    # plt.colorbar(CS, norm=norm)

    path_colors = ['red', 'orange', 'yellow', 'white']

    # Plot current firefronts to the first 3 subplots
    for p, path in enumerate(pathes):
        patch = mpatches.PathPatch(path, edgecolor=path_colors[p % len(path_colors)], facecolor='none', alpha=1, lw=2)
        ax.add_patch(patch)
        ax.grid()
        ax.axis('equal')

    ax.grid()
    ax.axis('equal')
    plt.show()



def print_ff_script(file_path):
    """
    Executes each line of a .ff script file using the ff.execute() method,
    after stripping leading and trailing whitespace, including tabs.
    """
    # Open the file with the script
    with open(file_path, 'r') as file:
        # Iterate over each line in the file
        for line in file:
            # Strip leading and trailing whitespace, including tabs, from the line
            clean_line = line.strip()
            # Check if the cleaned line is not empty
            if clean_line:
                # Execute the cleaned line with ff.execute, ensuring no leading/trailing whitespace
                command = f'ff.execute("{clean_line}")'
                print(command)  # This is for demonstration; replace with actual ff.execute(clean_line) in use
                # If you have the ff object with an execute method available, you would call it here:
                # ff.execute(clean_line)
