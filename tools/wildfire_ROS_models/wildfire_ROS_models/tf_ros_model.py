#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Nov 23 12:25:51 2023

Wildfire ROS Model - Machine learning emulator

Description:
This module contains a class to use a multilayered interpolator of a model,
and methods to generate data and train it

Author: Jean-Baptiste Filippi
Organization: CNRS
License: GPL

"""

import numpy as np
import tensorflow as tf
from tensorflow.keras.callbacks import EarlyStopping, ModelCheckpoint, ReduceLROnPlateau
from .model_set import model_parameters
import struct

def train_wildfire_speed_emulator(model, problem, model_path, config):
    # Extracting data
    X = problem["input"]  # Feature matrix
    y = problem["results"]  # Target values

    l1_reg = tf.keras.regularizers.l1(config["l1_reg_coeff"])
    for layer in model.layers:
        layer.kernel_regularizer = l1_reg

    # Define SGD algorithm
    optimizer = tf.keras.optimizers.Adam(learning_rate=config["learning_rate"])

    # Compile the model for regression
    model.compile(optimizer=optimizer, loss=config["loss"])

    # Stop optimization when validation loss increases
    earlyStopping = EarlyStopping(
        monitor="val_loss", patience=config["patience"], verbose=0, mode="min"
    )

    # Save model if validation loss is improved
    nn_file_name = config["model_path"] + "/nn.keras"
    print(f"saving {nn_file_name}")
    mcp_save = ModelCheckpoint(
        nn_file_name, save_best_only=True, monitor="val_loss", mode="min"
    )

    # Reduce learning rate when validation loss reaches a plateau
    reduce_lr_loss = ReduceLROnPlateau(
        monitor="val_loss",
        factor=config["lr_scheduler"]["factor"],
        patience=config["lr_scheduler"]["patience"],
        verbose=1,
        min_delta=config["lr_scheduler"]["min_delta"],
        mode="min",
    )

    # Train the model
    model.fit(
        X["train"],
        y["train"],
        epochs=config["epochs"],
        batch_size=config["batch_size"],
        validation_data=(X["val"], y["val"]),
        callbacks=[earlyStopping, mcp_save, reduce_lr_loss],
    )

    model.save(nn_file_name)  # Saving the model
    return model


def add_results_emulation(problem, model):
    # Prepare input data - assuming 'input' is already in the correct format
    problem["results_emulation"] = {}
    input_data = problem["input"]

    # Check if input_data dimensions match num_vars
    if input_data.shape[1] != problem["num_vars"]:
        raise ValueError("Input data dimensions do not match the number of variables")

    # Make predictions
    predictions = model.predict(input_data)

    # Update the problem dictionary with predictions
    problem[
        "results_emulation"
    ] = predictions.flatten()  # Flatten in case predictions are in 2D array

    return problem


def predict_single_output(model_path, input_dict, paramset):
    # Load the trained model
    model = tf.keras.models.load_model(model_path)

    # Prepare the input data in the correct order
    input_order = input_dict["names"]
    input_values = np.array([input_dict["input"][name] for name in input_order])

    # Reshape input_values for a single prediction
    input_values = input_values.reshape(1, -1)

    # Make the prediction
    prediction = model.predict(input_values)

    # Return the scalar result
    return model_parameters({"ROS": prediction[0, 0]})

def save_model_structure(model, filename, input_names=None, output_names=None):
    with open(filename, "wb") as file:
        header_format = "8s i"
        nlayers = len(model.layers)
        file.write(struct.pack(header_format, b"FFANN001", nlayers))

        for layer in model.layers:
            # Check the type of each layer and handle accordingly
            if isinstance(layer, tf.keras.layers.Dense):
                weights_biases = layer.get_weights()
                weights, biases = weights_biases
                activation = layer.get_config().get("activation", "NONE")
                activation_code = {
                    "relu": b"RELU",
                    "sigmoid": b"SIGM",
                    "linear": b"LINE",
                }.get(activation, b"NONE")
                layer_format = "4s ii"
                file.write(
                    struct.pack(
                        layer_format,
                        activation_code,
                        weights.shape[0],
                        weights.shape[1],
                    )
                )
                weights_format = f"{weights.size}f"
                biases_format = f"{biases.size}f"
                file.write(struct.pack(weights_format, *weights.flatten()))
                file.write(struct.pack(biases_format, *biases.flatten()))

            elif isinstance(layer, tf.keras.layers.Normalization):
                # Handle normalization layers specifically
                mean = layer.mean.numpy().flatten()
                variance = layer.variance.numpy().flatten()

                layer_format = "4s ii"  # Example format, adjust as necessary
                activation_code = b"NORM"
                # print("creating norm ",activation_code, layer.mean, layer.variance)
                file.write(
                    struct.pack(layer_format, activation_code, mean.size, variance.size)
                )
                mean_format = f"{mean.size}f"
                variance_format = f"{variance.size}f"
                file.write(struct.pack(mean_format, *mean))
                file.write(struct.pack(variance_format, *variance))

            else:
                # For layers that do not match any above (e.g., Dropout, Flatten)
                activation_code = b"NONE"
                layer_format = "4s ii"
                file.write(struct.pack(layer_format, activation_code, 0, 0))

        # Save input and output names at the end of the file
        input_names_bytes = ",".join(input_names).encode() if input_names else b""
        output_names_bytes = ",".join(output_names).encode() if output_names else b""
        file.write(struct.pack("i", len(input_names_bytes)))
        file.write(input_names_bytes)
        file.write(struct.pack("i", len(output_names_bytes)))
        file.write(output_names_bytes)


def load_model_structure(filename, new_normalisation_data=None):
    with open(filename, "rb") as file:
        # Read and unpack the header
        header_format = "8s i"
        header_size = struct.calcsize(header_format)
        header = file.read(header_size)
        magic, nlayers = struct.unpack(header_format, header)
        print(">>>>>>", magic, nlayers)
        # Prepare to reconstruct the model
        model = tf.keras.Sequential()

        if magic != b"FFANN001":
            raise ValueError("Invalid file format or magic number.")

        if nlayers == 0:
            raise ValueError("Model has no layers.")
        
        # Read and reconstruct each layer
        layers_info = []
        for _ in range(nlayers):
            layer_info = file.read(struct.calcsize("4s ii"))
            activation_code, num_inputs, num_outputs = struct.unpack("4s ii", layer_info)
            print(">>>>>>>>>>>", activation_code, num_inputs, num_outputs)
            if activation_code in [b"RELU", b"SIGM", b"LINE"]:
                # Standard dense layers
                activation = {
                    b"RELU": "relu",
                    b"SIGM": "sigmoid",
                    b"LINE": "linear",
                    b"NONE": None,
                }.get(
                    activation_code, "linear"
                )  # Default to linear if unknown code

                weights_size = num_inputs * num_outputs
                weights_format = f"{weights_size}f"
                weights = np.array(
                    struct.unpack(weights_format, file.read(weights_size * 4)),
                    dtype=np.float32,
                ).reshape((num_inputs, num_outputs))

                # Read biases
                biases_format = f"{num_outputs}f"
                biases = np.array(
                    struct.unpack(biases_format, file.read(num_outputs * 4)),
                    dtype=np.float32,
                )

                layers_info.append((activation_code, num_inputs, num_outputs,weights,biases))

            elif activation_code == b"NORM":
                # Normalization layers
                mean_size = num_inputs  # Using num_inputs for mean size
                variance_size = num_outputs  # Using num_outputs for variance size

                mean_format = f"{mean_size}f"
                variance_format = f"{variance_size}f"

                mean = np.array(
                    struct.unpack(mean_format, file.read(mean_size * 4)),
                    dtype=np.float32,
                )
                variance = np.array(
                    struct.unpack(variance_format, file.read(variance_size * 4)),
                    dtype=np.float32,
                )

                layers_info.append((activation_code, num_inputs, num_outputs,mean,variance))

            else:
                # Other layers could be handled here as needed
                continue

        model = tf.keras.Sequential()

        # 4. Add the explicit Input layer based on the first layer's input size
        first_layer_info = layers_info[0]
        _, first_num_inputs, _ ,_,_= first_layer_info
        model.add(tf.keras.Input(shape=(first_num_inputs,)))
        
        # 6. Iterate over each layer to reconstruct the model
        for layer_idx in range(nlayers):
            layer_info = layers_info[layer_idx]
            activation_code, num_inputs, num_outputs, weights, biases = layer_info

            if activation_code in [b"RELU", b"SIGM", b"LINE"]:
            
                activation = {
                    b"RELU": "relu",
                    b"SIGM": "sigmoid",
                    b"LINE": "linear",
                    b"NONE": None,
                }.get(activation_code, "linear")  # Default to linear if unknown

                # d. Create and add the Dense layer
                layer = tf.keras.layers.Dense(num_outputs, activation=activation)
                model.add(layer)

                # e. Set weights and biases
                layer.set_weights([weights, biases])

            elif activation_code == b"NORM":

                if new_normalisation_data is not None:
                    # Define the Normalization layers - Set axis=-1 to normalize per feature

                    num_samples, input_size = np.shape(new_normalisation_data)

                    # Generate random inputs for the model
                    inputs = np.random.rand(num_samples, input_size).astype(np.float32)
                    new_norm = new_normalisation_data.astype(np.float32)
                    inputs[:]= new_norm[:]
        
                    # Define the Normalization layers - Set axis=-1 to normalize per feature
                    input_normalizer = tf.keras.layers.Normalization(axis=-1)
                    input_normalizer.build(input_shape=(None, num_inputs))
                    # Adapt normalizers to the dataset
                    input_normalizer.adapt(inputs)

                    model.add(input_normalizer)
                else:
                    # a. Read mean and variance for Normalization layer
                    mean_size = num_inputs
                    variance_size = num_outputs
                    mean = weights
                    variance = biases
                    # **Assuming `center=True` and `scale=False`**
                    # Adjust these parameters based on your actual layer configuration
                    layer = tf.keras.layers.Normalization(axis=-1)

                    # Build the layer to initialize variables
                    layer.build(input_shape=(None, num_inputs))

                    # Manually assign the mean and variance to the layer's internal variables
                    layer.mean = tf.convert_to_tensor([mean], dtype=tf.float32)
                    layer.variance = tf.convert_to_tensor([variance], dtype=tf.float32)

                    # Add the Normalization layer to the model
                    model.add(layer)


            else:

                print(f"Skipping unknown layer type with code: {activation_code}")
                # You may need to read additional bytes depending on the layer type
                pass


        # Read input and output names
        input_names_len = struct.unpack("i", file.read(4))[0]
        input_names_bytes = file.read(input_names_len)
        input_names = input_names_bytes.decode().split(",") if input_names_bytes else []

        output_names_len = struct.unpack("i", file.read(4))[0]
        output_names_bytes = file.read(output_names_len)
        output_names = (
            output_names_bytes.decode().split(",") if output_names_bytes else []
        )

        return model, input_names, output_names


""" 
def load_model_structure(filename):
    with open(filename, "rb") as file:
        # Read and unpack the header
        header_format = "8s i"
        header_size = struct.calcsize(header_format)
        header = file.read(header_size)
        _, nlayers = struct.unpack(header_format, header)

        # Prepare to reconstruct the model
        model = tf.keras.Sequential()

        # Read and reconstruct each layer
        layer_format = "4s ii"
        for _ in range(nlayers):
            layer_info = file.read(struct.calcsize(layer_format))
            activation_code, num_inputs, num_outputs = struct.unpack(
                layer_format, layer_info
            )
            # print("read ",activation_code, num_inputs, num_outputs)
            # Decide how to process based on activation_code
            if activation_code in [b"RELU", b"SIGM", b"LINE"]:
                # Standard dense layers
                activation = {
                    b"RELU": "relu",
                    b"SIGM": "sigmoid",
                    b"LINE": "linear",
                    b"NONE": None,
                }.get(
                    activation_code, "linear"
                )  # Default to linear if unknown code

                weights_size = num_inputs * num_outputs
                weights_format = f"{weights_size}f"
                weights = np.array(
                    struct.unpack(weights_format, file.read(weights_size * 4)),
                    dtype=np.float32,
                ).reshape((num_inputs, num_outputs))

                # Read biases
                biases_format = f"{num_outputs}f"
                biases = np.array(
                    struct.unpack(biases_format, file.read(num_outputs * 4)),
                    dtype=np.float32,
                )

                # Create layer with weights and activation
                layer = tf.keras.layers.Dense(
                    num_outputs, activation=activation, input_shape=(num_inputs,)
                )

                model.add(layer)
                model.build(input_shape=(None, num_inputs))
                # Set weights manually
                layer.set_weights([weights, biases])

            elif activation_code == b"NORM":
                # Normalization layers
                mean_size = num_inputs  # Using num_inputs for mean size
                variance_size = num_outputs  # Using num_outputs for variance size

                mean_format = f"{mean_size}f"
                variance_format = f"{variance_size}f"

                mean = np.array(
                    struct.unpack(mean_format, file.read(mean_size * 4)),
                    dtype=np.float32,
                )
                variance = np.array(
                    struct.unpack(variance_format, file.read(variance_size * 4)),
                    dtype=np.float32,
                )

                # Define the Normalization layer
                layer = tf.keras.layers.Normalization(axis=-1)

                # Build the layer first so that it initializes the mean and variance
                layer.build(input_shape=(None, num_inputs))

                # Manually assign the mean and variance to the layer's internal variables
                layer.mean = tf.convert_to_tensor([mean], dtype=tf.float32)
                layer.variance = tf.convert_to_tensor([variance], dtype=tf.float32)

                # print("new norm ",activation_code, layer.mean, layer.variance)
                model.add(layer)

            else:
                # Other layers could be handled here as needed
                continue

        # Read input and output names
        input_names_len = struct.unpack("i", file.read(4))[0]
        input_names_bytes = file.read(input_names_len)
        input_names = input_names_bytes.decode().split(",") if input_names_bytes else []

        output_names_len = struct.unpack("i", file.read(4))[0]
        output_names_bytes = file.read(output_names_len)
        output_names = (
            output_names_bytes.decode().split(",") if output_names_bytes else []
        )

        return model, input_names, output_names
 """