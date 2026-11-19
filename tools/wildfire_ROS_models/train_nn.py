""" Script to train a ROS model emulator
Ex: python train_nn.py --root /home/ai4geo/Documents/nn_ros_models --target_ros_model RothermelAndrews2018 --n_samples 10000 --epochs 200 --overwrite
"""

from wildfire_ROS_models.sensitivity import generate_problem_set
from wildfire_ROS_models.tf_ros_model import *
from wildfire_ROS_models.ffann import export_ffann, check_ffann
from wildfire_ROS_models.utils import *

import os
import sys
import logging
import argparse

import time


logging.basicConfig(stream=sys.stdout, level=logging.INFO)
logger = logging.getLogger()


def main(args):
    target_ros_model = args.target_ros_model
    nn_model_path = os.path.join(args.root, "nn_" + target_ros_model)
    n_train_samples = int(args.n_samples)
    train_data_path = os.path.join(
        args.root, target_ros_model + f"_train_data_n_samples_{n_train_samples}.pkl"
    )

    train_config = {
        "optimizer": "adam",
        "loss": "mean_absolute_error",
        "h_dim": 64,
        "n_hidden_layers": 2,
        "epochs": args.epochs,
        "batch_size": args.batch_size,
        "l1_reg_coeff": args.l1_reg_coeff,
        "val_prop": args.val_prop,
        "learning_rate": args.lr,
        "patience": args.patience,
        "model_path": nn_model_path,
        "lr_scheduler": {"factor": 0.5, "patience": 5, "min_delta": 1e-4},
    }

    # Create the training data:
    #   - Input data is sampled with Sobol indices
    #   - Target data is computed with target_ros_model, e.g. Rothermel
    if os.path.exists(train_data_path):
        logger.info(f"Load training data set from {train_data_path}")
        train_set = load_pkl(train_data_path)
    else:
        logger.info("Create training data set")
        stime = time.time()
        train_set = generate_problem_set(
            target_ros_model,
            N=n_train_samples,
            val_prop=train_config["val_prop"],
            selected_params=args.selected_params,
            bounds=args.bounds,
        )
        ptime = (time.time() - stime) / 60
        logger.info(f"Built data in {ptime:.2f}min")
        if train_data_path is not None:
            save_to_pkl(train_set, train_data_path)
            logger.info(f"Training data set saved in {train_data_path}")

    # Define a neural network model suitable for regression
    normalization_layer = tf.keras.layers.Normalization(axis=-1)
    normalization_layer.adapt(train_set["input"]["train"])

    model_layers = [
        normalization_layer,
        tf.keras.layers.Dense(
            train_config["h_dim"],
            activation="relu",
            input_shape=(train_set["num_vars"],),
        ),
    ]

    for _i in range(1, train_config["n_hidden_layers"]):
        model_layers.append(
            tf.keras.layers.Dense(train_config["h_dim"], activation="relu")
        )
    model_layers.append(tf.keras.layers.Dense(1))

    model = tf.keras.Sequential(model_layers)

    # Train a dense neural network
    save_to_json(train_config, os.path.join(nn_model_path, "config.json"))

    keras_path = os.path.join(nn_model_path, "nn.keras")
    if args.overwrite or not os.path.exists(keras_path):
        logger.info("Optimize neural network")
        stime = time.time()
        train_wildfire_speed_emulator(model, train_set, nn_model_path, train_config)
        ptime = (time.time() - stime) / 60
        logger.info(f"Optimized NN in {ptime:.2f}min")
    else:
        logger.info(
            f"Neural network has already been trained - params saved in {nn_model_path}"
        )
        model = tf.keras.models.load_model(keras_path)

    ffann_path = os.path.join(nn_model_path, target_ros_model + ".ffann")
    names = export_ffann(model, ffann_path, train_set["names"])
    diff = check_ffann(model, ffann_path, train_set["input"]["val"])
    logger.info(f"Wrote {ffann_path} (inputs {', '.join(names)}; max diff {diff:.2e})")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=str, help="Path to your root folder")
    parser.add_argument(
        "--target_ros_model", type=str, default="Rothermel1972", help="Model to emulate"
    )
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
    args.selected_params = None
    args.bounds = None

    if args.target_ros_model == "RothermelAndrews2018":
        # Named as ForeFire's RothermelAndrews2018 reads them, so the network
        # loads there (see wildfire_ROS_models/ffann.py). Wind is sampled
        # negative too: the back of the front sees wind against it.
        args.bounds = {"wind": [-10, 10]}
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
