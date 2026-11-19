"""
Created on Wed Sep 27 17:50:20 2023

fuel and models package
"""
# __init__.py

# Importing submodules to make them accessible via the package namespace
from . import fuels_database
from . import interactive_polar_plot
from . import model_set
from . import py_ROS_models_to_forefire_cpp
from . import runROS
from . import sensitivity
from . import utils
from . import Balbi2020
from . import Cruz
from . import Rothermel1972
from . import RothermelAndrews2018

import importlib

# These need TensorFlow (`uv sync --extra nn`), so they load on first use and
# the models above run without it.
_NEEDS_TENSORFLOW = {"tf_ros_model", "tf_helpers", "neuralNetROS", "ffann"}


def __getattr__(name):
    if name in _NEEDS_TENSORFLOW:
        return importlib.import_module(f".{name}", __name__)
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")