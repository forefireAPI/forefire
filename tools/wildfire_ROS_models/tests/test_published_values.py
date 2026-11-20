"""Rate of spread against values published in Andrews (2018), RMRS-GTR-371.

Table 17, first row: one dead 1-h class, 2 ton/ac, SAV 2500 ft2/ft3, depth 1 ft,
moisture of extinction 20 %, fuel moisture 5 %, midflame wind 5 mi/h, no slope.
Published: wind factor 17.8, rate of spread 81.6 ft/min. Heat content, particle
density and mineral content are the standard values of Andrews (2018), table 5.
"""

import pytest

from wildfire_ROS_models.model_set import model_parameters
from wildfire_ROS_models.Rothermel1972 import Rothermel1972
from wildfire_ROS_models.RothermelAndrews2018 import RothermelAndrews2018

TABLE_17_ROW_1 = {
    "SAVcar_ftinv": 2500,
    "fl1h_tac": 2.0,
    "fd_ft": 1.0,
    "Dme_r": 0.20,
    "mdOnDry1h_r": 0.05,
    "wind_miph": 5,
    "slope_deg": 0,
    "H_BTUlb": 8000,
    "fuelDens_lbft3": 32,
    "totMineral_r": 0.0555,
    "effectMineral_r": 0.01,
}
PUBLISHED_ROS_FTMIN = 81.6


@pytest.mark.parametrize("model", [Rothermel1972, RothermelAndrews2018])
def test_ros_andrews2018_table17(model):
    ros = model(model_parameters(TABLE_17_ROW_1))["ROS_ftmin"]
    assert ros == pytest.approx(PUBLISHED_ROS_FTMIN, rel=0.005)
