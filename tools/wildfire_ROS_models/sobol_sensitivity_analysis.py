""" Script to train a ROS model emulator and perform a sensibility analysis
Ex: python sobol_sensitivity_analysis.py --root /home/ai4geo/Documents/nn_ros_models --target_ros_model RothermelAndrews2018 --n_samples 10000
"""

from wildfire_ROS_models.sensitivity import (
    generate_problem_set,
    plot_sobol_indices,
    sobol_analysis,
)
from wildfire_ROS_models.tf_ros_model import *
from wildfire_ROS_models.utils import *

import sys
import logging
import argparse

import matplotlib.pyplot as plt

import time

import pdb


logging.basicConfig(stream=sys.stdout, level=logging.INFO)
logger = logging.getLogger()


def main(args):
    target_ros_model = args.target_ros_model
    if args.nn_model_path is not None:
        nn_model_path = args.nn_model_path
    else:
        nn_model_path = os.path.join(args.root, "nn_" + target_ros_model)
    n_train_samples = int(args.n_samples)

    logger.info("Sample in input space with Sobol sequences")
    stime = time.time()
    problem_set = generate_problem_set(
        target_ros_model, N=n_train_samples, selected_params=args.selected_params
    )
    ptime = (time.time() - stime) / 60
    logger.info(f"Sampled data in {ptime:.2f}min")

    # Sensibility analysis of the target ROS model
    logger.info("Run sensibility analysis of target ROS model")
    Si_ros, params, y_pos, model_name = sobol_analysis(problem_set, lookat="results")
    plot_sobol_indices(Si_ros, params, y_pos, model_name)

    # Load neural network
    if nn_model_path.split(".")[-1] == "ffann":
        model, input_names, output_names = load_model_structure(nn_model_path)
    else:
        model = tf.keras.saving.load_model(nn_model_path)
    # import pdb; pdb.set_trace()
    # Prediction on validation set
    logger.info("Compute predictions")
    problem_set = add_results_emulation(problem_set, model)

    # Sensibility analysis of the emulator
    logger.info("Run sensibility analysis of NN model")
    Si_nn, params, y_pos, model_name = sobol_analysis(
        problem_set, lookat="results_emulation"
    )

    plot_sobol_indices(Si_nn, params, y_pos, model_name)
    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=str, help="Path to your root folder")
    parser.add_argument(
        "--target_ros_model", type=str, default="Rothermel1972", help="Model to emulate"
    )
    parser.add_argument("--nn_model_path", type=str, help="Path to trained NN")
    parser.add_argument(
        "--n_samples",
        type=float,
        default=2**15,
        help="Number of training data points",
    )
    parser.add_argument("--epochs", type=int, default=100, help="Number of epochs")
    parser.add_argument(
        "--batch_size", type=int, default=128, help="Batch size for SGD"
    )
    parser.add_argument("--lr", type=float, default=1e-3, help="SGD learning rate")
    parser.add_argument(
        "--l1_reg_coeff",
        type=float,
        default=1e-2,
        help="Coefficient of L1 norm regularization (enforces sparsity)",
    )
    parser.add_argument(
        "--val_prop",
        type=float,
        default=0.2,
        help="Percentage of training data for validation",
    )
    parser.add_argument(
        "--patience",
        type=int,
        default=10,
        help="Number of epochs after which training is stopped if validation loss keeps increasing",
    )
    parser.add_argument(
        "--overwrite", action="store_true", help="Whether to overwrite trained model"
    )
    args = parser.parse_args()

    if args.target_ros_model == "RothermelAndrews2018":
        args.selected_params = [
            "fl1h_tac",
            "fd_ft",
            "Dme_pc",
            "SAVcar_ftinv",
            "mdOnDry1h_r",
            "wind",
            "slope_tan",
        ]

    main(args)
