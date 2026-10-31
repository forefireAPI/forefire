#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Sensitivity Analysis Script for wildfire_ROS_models Models

This script provides helper functions to perform and plot sensitivity analysis
using the Sobol method.

Created on Wed Nov 22 12:14:58 2023

Author: filippi_j, Thoreau_r
"""

import argparse
import matplotlib.pyplot as plt
import numpy as np
from sklearn.model_selection import train_test_split
from SALib.analyze import sobol
from SALib.sample import sobol as sobolsample
from wildfire_ROS_models.runROS import ROS_models
from wildfire_ROS_models.model_set import model_parameters, var_properties


def generate_problem_set(
    model_key,
    kind_of_parameter=["environment", "typical", "fuelstate", "model"],
    result_var="ROS",
    N=4096,
    val_prop=None,
    selected_params=None,
):
    """
    Generate a problem set for sensitivity analysis using the Sobol method.

    Parameters:
        model_key (str): Key identifying the ROS model.
        kind_of_parameter (list): List of parameter categories to include.
        result_var (str): The result variable to analyze.
        N (int): Number of samples to generate.
        val_prop (float): Proportion of validation data.
        selected_params (list): selection of parameters to use.

    Returns:
        dict: A dictionary containing the problem setup and results.
    """
    modelVSet = ROS_models[model_key]["get_set"]()

    fm = {}

    for key in modelVSet.keys():
        for var_key in modelVSet[key]:

            fm[var_key] = modelVSet[key][var_key]

    fm_var_set = {}

    for key in kind_of_parameter:
        for var_key in modelVSet[key]:
            fm_var_set[var_key] = modelVSet[key][var_key]

    sh = model_parameters(fm_var_set)

    if selected_params is not None:
        ordered_fm_var_set = {k: sh[k] for k in selected_params}
        fm_var_set = ordered_fm_var_set
        del ordered_fm_var_set

    s_properties = model_parameters.get_specialized_properties_set(fm_var_set.keys())

    problem = {
        "model_name": model_key,
        "num_vars": len(fm_var_set.keys()),
        "names": list(fm_var_set.keys()),
        "bounds": [s_properties[name]["range"] for name in fm_var_set.keys()],
    }

    print("SSSSS ", problem["bounds"], fm_var_set.keys())
    param_values = sobolsample.sample(problem, N)
    problem["input"] = param_values

    model_results = []
    for params in param_values:
        for i, param_name in enumerate(problem["names"]):
            fm[param_name] = params[i]
        fm = model_parameters(fm)
        result = model_parameters(ROS_models[model_key]["get_values"](fm))

        model_results.append(result[result_var])

    problem["result_var"] = result_var
    problem["results"] = np.array(model_results)

    if val_prop is not None:
        X_train, X_val, y_train, y_val = train_test_split(
            problem["input"], problem["results"], test_size=val_prop
        )
        problem["input"] = {"train": X_train, "val": X_val}
        problem["results"] = {"train": y_train, "val": y_val}

    return problem


def verify_error(problem_set, lookat="results"):
    """
    Verify the error between the model's results and the generated problem set.

    Parameters:
        problem_set (dict): The problem set containing inputs and results.
        lookat (str): The key to look at in the problem set.

    Returns:
        float: The average absolute error.
    """
    model_key = problem_set["model_name"]
    modelVSet = ROS_models[model_key]["get_set"]()
    fm = model_parameters(modelVSet)
    diff = 0
    numSamples = len(problem_set[lookat])

    #  for key in modelVSet.keys():
    #     fm = fm + modelVSet[key]

    for i in range(numSamples):
        input_values = problem_set["input"][i]
        for j, param_name in enumerate(problem_set["names"]):
            fm[param_name] = input_values[j]

        result = model_parameters(ROS_models[model_key]["get_values"](fm))
        diff += abs(result[problem_set["result_var"]] - problem_set[lookat][i])

    return diff / numSamples


def sobol_analysis(problem_set, lookat="results"):
    """
    Perform Sobol sensitivity analysis on the problem set.

    Parameters:
        problem_set (dict): The problem set containing inputs and results.
        lookat (str): The key to look at in the problem set.

    Returns:
        tuple: Sobol indices, parameter names, y positions, and model name.
    """
    Si = sobol.analyze(problem_set, problem_set[lookat])
    params = problem_set["names"]
    model_name = problem_set["model_name"]
    y_pos = np.arange(len(params))
    return Si, params, y_pos, model_name


def plot_sobol_indices(Si, params, y_pos, model_name):
    """
    Plot the Sobol sensitivity indices.

    Parameters:
        Si (dict): Sobol indices.
        params (list): List of parameter names.
        y_pos (numpy.ndarray): Positions for the y-axis.
        model_name (str): Name of the model.
    """
    plt.figure(figsize=(10, 5))

    # Plotting first-order indices
    plt.subplot(1, 2, 1)
    plt.barh(y_pos, Si["S1"], align="center", color="skyblue")
    plt.yticks(y_pos, params)
    plt.xlabel("First-order Sobol Index")
    plt.title(f"First-order {model_name}")

    # Plotting total-effect indices
    plt.subplot(1, 2, 2)
    plt.barh(y_pos, Si["ST"], align="center", color="salmon")
    plt.yticks(y_pos, params)

    plt.xlabel("Total-effect Sobol Index")
    plt.title(f"Total-effect {model_name}")

    plt.tight_layout()
    plt.show()


def plot_sobol_indices(Si, params, y_pos, model_name):
    """
    Plot the Sobol sensitivity indices.

    Parameters:
        Si (dict): Sobol indices.
        params (list): List of parameter names.
        y_pos (numpy.ndarray): Positions for the y-axis.
        model_name (str): Name of the model.
    """
    plt.figure(figsize=(10, 5))

    # Plotting first-order indices
    plt.subplot(1, 2, 1)
    plt.barh(y_pos, Si["S1"], align="center", color="skyblue")
    plt.yticks(y_pos, params)
    plt.xlabel("First-order Sobol Index")
    plt.title(f"First-order {model_name}")

    # Plotting total-effect indices
    plt.subplot(1, 2, 2)
    plt.barh(y_pos, Si["ST"], align="center", color="salmon")
    plt.yticks(y_pos, params)
    plt.xlabel("Total-effect Sobol Index")
    plt.title(f"Total-effect {model_name}")

    plt.tight_layout()
    plt.show()


def main():
    """
    Main function to perform sensitivity analysis.
    """
    parser = argparse.ArgumentParser(
        description="Perform sensitivity analysis on wildfire_ROS_models models."
    )
    parser.add_argument(
        "--model",
        type=str,
        default="RothermelAndrews2018",
        help='Model key (e.g., "RothermelAndrews2018")',
    )
    parser.add_argument(
        "--N", type=int, default=2**10, help="Number of samples for Sobol analysis"
    )
    parser.add_argument(
        "--val_prop", type=float, default=None, help="Proportion of validation data"
    )
    parser.add_argument(
        "--result_var", type=str, default="ROS", help="Result variable to analyze"
    )
    parser.add_argument(
        "--plot", action="store_true", help="Whether to plot the Sobol indices"
    )

    args = parser.parse_args()
    selected_params = [
        "fl1h_tac",
        "fd_ft",
        "Dme_pc",
        "SAVcar_ftinv",
        "mdOnDry1h_r",
        "wind",
        "slope_tan",
    ]
    # Generate problem set
    problem_set = generate_problem_set(
        model_key=args.model,
        result_var=args.result_var,
        N=args.N,
        val_prop=args.val_prop,
        selected_params=selected_params,
    )

    # Perform Sobol analysis
    Si_ros, params, y_pos, model_name = sobol_analysis(problem_set)

    # Optionally plot the results
    if args.plot:
        plot_sobol_indices(Si_ros, params, y_pos, model_name)
    plot_sobol_indices(Si_ros, params, y_pos, model_name)

    # Optionally verify error
    error = verify_error(problem_set)
    print(f"Average Absolute Error: {error}")


if __name__ == "__main__":
    main()
