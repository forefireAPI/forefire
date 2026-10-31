#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Sensitivity Analysis Script for wildfire_ROS_models Models

This script trains a ROS model emulator and performs sensitivity analysis using the Sobol method.

Example Usage:
    python sensitivity_analysis.py --root /home/ai4geo/Documents/nn_ros_models --target_ros_model RothermelAndrews2018 --n_samples 10000
"""

import os
import sys
import logging
import argparse
import time

import matplotlib.pyplot as plt

from wildfire_ROS_models.sensitivity import (
    generate_problem_set,
    plot_sobol_indices,
    sobol_analysis,
)
from wildfire_ROS_models.tf_ros_model import load_model_structure, add_results_emulation
from wildfire_ROS_models.utils import *

import tensorflow as tf  # Ensure TensorFlow is installed

# Configure logging
logging.basicConfig(stream=sys.stdout, level=logging.INFO)
logger = logging.getLogger(__name__)


def main(args):
    target_ros_model = args.target_ros_model
    if args.nn_model_path is not None:
        nn_model_path = args.nn_model_path
    else:
        nn_model_path = os.path.join(args.root, f"nn_{target_ros_model}")
    n_train_samples = int(args.n_samples)

    logger.info("Sampling in input space with Sobol sequences")
    start_time = time.time()
    problem_set = generate_problem_set(target_ros_model, N=n_train_samples)
    sampling_time = (time.time() - start_time) / 60
    logger.info(f"Sampled data in {sampling_time:.2f} minutes")

    # Sensitivity analysis of the target ROS model
    logger.info("Running sensitivity analysis of target ROS model")
    Si_ros, params, y_pos, model_name = sobol_analysis(problem_set, lookat="results")
    plot_sobol_indices(Si_ros, params, y_pos, model_name)

    # Load neural network emulator
    logger.info("Loading neural network emulator")
    if nn_model_path.endswith(".ffann"):
        model, input_names, output_names = load_model_structure(nn_model_path)
    else:
        model = tf.keras.models.load_model(nn_model_path)

    # Prediction on validation set
    logger.info("Computing predictions using the emulator")
    problem_set = add_results_emulation(problem_set, model)

    # Sensitivity analysis of the emulator
    logger.info("Running sensitivity analysis of neural network emulator")
    Si_nn, params_nn, y_pos_nn, model_name_nn = sobol_analysis(
        problem_set, lookat="results_emulation"
    )

    plot_sobol_indices(Si_nn, params_nn, y_pos_nn, model_name_nn)
    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Train a ROS model emulator and perform sensitivity analysis."
    )
    parser.add_argument(
        "--root",
        type=str,
        required=True,
        help="Path to your root folder containing neural network models",
    )
    parser.add_argument(
        "--target_ros_model",
        type=str,
        default="RothermelAndrews2018",
        help="Model to emulate",
    )
    parser.add_argument(
        "--nn_model_path", type=str, help="Path to trained neural network emulator"
    )
    parser.add_argument(
        "--n_samples", type=int, default=32768, help="Number of training data points"
    )
    parser.add_argument(
        "--epochs",
        type=int,
        default=100,
        help="Number of epochs for training (if applicable)",
    )
    parser.add_argument(
        "--batch_size", type=int, default=128, help="Batch size for training"
    )
    parser.add_argument(
        "--lr", type=float, default=1e-3, help="Learning rate for training"
    )
    parser.add_argument(
        "--l1_reg_coeff", type=float, default=1e-2, help="L1 regularization coefficient"
    )
    parser.add_argument(
        "--val_prop", type=float, default=0.2, help="Proportion of data for validation"
    )
    parser.add_argument(
        "--patience", type=int, default=10, help="Patience for early stopping"
    )
    parser.add_argument(
        "--overwrite", action="store_true", help="Whether to overwrite trained model"
    )

    args = parser.parse_args()
    main(args)
