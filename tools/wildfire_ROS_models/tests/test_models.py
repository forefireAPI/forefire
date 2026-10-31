#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Tests for wildfire_ROS_models models.

Created on Mon Nov 20 08:01:29 2023

Author: filippi_j
"""

import pytest
import matplotlib.pyplot as plt
import numpy as np
from wildfire_ROS_models import fuels_database as fdb
from wildfire_ROS_models.runROS import run_model, plot_results
from wildfire_ROS_models.model_set import model_parameters
from wildfire_ROS_models.sensitivity import (
    plot_sobol_indices,
    generate_problem_set,
    sobol_analysis,
)


@pytest.fixture(scope="module")
def setup_plotting():
    """Fixture to configure matplotlib for testing."""
    plt.ioff()  # Disable interactive plotting
    yield
    plt.close("all")  # Close all plots after tests


def test_figure_9_AndrewsRothermel2017(setup_plotting):
    """Test for figure 9 using Andrews & Rothermel 2017 model."""
    A2017 = fdb.load_csv(fdb.AR2017_table_csv)
    A2017_any = fdb.load_csv(fdb.AR2017_anyfueltable_csv)

    environment = model_parameters(
        {
            "wind_miph": 10,
            "slope_deg": 0,
            "mdOnDry1h_r": 0.06,
            "mdOnDry10h_r": 0.07,
            "mdOnDry100h_r": 0.08,
            "mdOnDryLHerb_r": 0.6,
            "mdOnDryLWood_r": 0.9,
        }
    )

    models = [
        run_model(
            "RothermelAndrews2018",
            fm + A2017_any[0] + environment,
            "wind_miph",
            np.arange(0, 20, 0.1),
        )
        for fm in A2017
    ]

    # Assert that models return expected structure
    for model in models:
        assert "results" in model, "Model output missing 'results' key."
        assert (
            "ROS_ftmin" in model["results"].columns
        ), "Missing 'ROS_ftmin' column in results."

    # Test plotting does not raise exceptions
    try:
        plot_results(models, "wind_miph", "ROS_ftmin")
    except Exception as e:
        pytest.fail(f"plot_results raised an exception: {e}")


def test_figure_3_4_Balbi2020(setup_plotting):
    """Test for figures 3 and 4 using Balbi 2020 model."""
    pn = fdb.load_csv(fdb.pineNeedlesBalbi2020_csv)[0] + model_parameters(
        {"wind_mps": 0, "slope_deg": 0}
    )

    # Test first set of parameters
    p_set = []
    for value in [0.08, 0.3, 0.8]:
        new_pn = model_parameters(pn.get_set())
        new_pn.fl1h_kgm2 = value
        new_pn.CODE = f"Pine Needles load {value}"
        p_set.append(new_pn)

    models = [
        run_model("Balbi2020", fm, "wind_mps", np.arange(0, 12, 1)) for fm in p_set
    ]

    # Assert model outputs
    for model in models:
        assert "results" in model, "Model output missing 'results' key."
        assert (
            "ROS_mps" in model["results"].columns
        ), "Missing 'ROS_mps' column in results."

    # Test plotting
    try:
        plot_results(models, "wind_mps", "ROS_mps")
    except Exception as e:
        pytest.fail(f"plot_results raised an exception: {e}")

    # Test second set of parameters
    p_set = []
    for value in [0.2, 8]:
        new_pn = model_parameters(pn.get_set())
        new_pn.fl1h_kgm2 = 0.9  # table. 2
        new_pn.wind_mps = value
        new_pn.CODE = f"Pine Needles wind {value}"
        p_set.append(new_pn)

    W02, W03 = [
        run_model("Balbi2020", fm, "mdOnDry1h_kgm2", np.arange(0.00, 0.9, 0.05))
        for fm in p_set
    ]

    # Compute RelativeRos
    W02["results"].RelativeRos = W02["results"].ROS / W02["results"].ROS.iloc[0]
    W03["results"].RelativeRos = W03["results"].ROS / W03["results"].ROS.iloc[0]

    # Assert computed RelativeRos
    for W in [W02, W03]:
        assert "results" in W, "Model output missing 'results' key."
        assert (
            "RelativeRos_r" in W["results"].columns
        ), "Missing 'RelativeRos_r' column in results."

    # Test plotting RelativeRos
    try:
        plot_results([W02, W03], "mdOnDry1h_kgm2", "RelativeRos_r")
    except Exception as e:
        pytest.fail(f"plot_results raised an exception: {e}")


def test_generate_problem_set():
    """Test the generation of the problem set."""
    problem_set = generate_problem_set(
        "RothermelAndrews2018",
        kind_of_parameter=["typical", "environment", "fuelstate"],
        result_var="ROS",
        N=10000,
    )

    assert len(problem_set) == 10000, f"Expected 10000 samples, got {len(problem_set)}."
    # Additional assertions based on the structure of problem_set can be added here


def test_sobol_analysis():
    """Test the Sobol sensitivity analysis."""
    problem_set = generate_problem_set(
        "RothermelAndrews2018",
        kind_of_parameter=["typical", "environment", "fuelstate"],
        result_var="ROS",
        N=10000,
    )
    Si_ros, params, y_pos, model_name = sobol_analysis(problem_set, lookat="results")

    assert Si_ros is not None, "Si_ros should not be None."
    assert len(params) > 0, "Parameter list should not be empty."
    assert len(y_pos) > 0, "y_pos should not be empty."
    assert (
        model_name == "RothermelAndrews2018"
    ), f"Expected model_name to be 'RothermelAndrews2018', got '{model_name}'."


def test_plot_sobol_indices(setup_plotting):
    """Test the plotting of Sobol indices."""
    problem_set = generate_problem_set(
        "RothermelAndrews2018",
        kind_of_parameter=["typical", "environment", "fuelstate"],
        result_var="ROS",
        N=10000,
    )
    Si_ros, params, y_pos, model_name = sobol_analysis(problem_set, lookat="results")

    # Test plotting Sobol indices
    try:
        plot_sobol_indices(Si_ros, params, y_pos, model_name)
    except Exception as e:
        pytest.fail(f"plot_sobol_indices raised an exception: {e}")
