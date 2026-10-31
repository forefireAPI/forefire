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


def train_wildfire_speed_emulator(model, problem, model_path, config):
    # Extracting data
    X = problem['input']  # Feature matrix
    y = problem['results']  # Target values

    l1_reg = tf.keras.regularizers.l1(config['l1_reg_coeff'])
    for layer in model.layers:
        layer.kernel_regularizer = l1_reg 

    # Define SGD algorithm
    optimizer = tf.keras.optimizers.Adam(learning_rate=config['learning_rate'])

    # Compile the model for regression
    model.compile(
        optimizer=optimizer,
        loss=config['loss']
        )
    
    # Stop optimization when validation loss increases
    earlyStopping = EarlyStopping(monitor='val_loss', patience=config['patience'], verbose=0, mode='min')
    
    # Save model if validation loss is improved
    mcp_save = ModelCheckpoint(config['model_path'], save_best_only=True, monitor='val_loss', mode='min')
    
    # Reduce learning rate when validation loss reaches a plateau
    reduce_lr_loss = ReduceLROnPlateau(
        monitor='val_loss', 
        factor=config['lr_scheduler']['factor'], 
        patience=config['lr_scheduler']['patience'], 
        verbose=1, 
        min_delta=config['lr_scheduler']['min_delta'], 
        mode='min'
        )
    
    # Train the model
    model.fit(
        X['train'], 
        y['train'], 
        epochs=config['epochs'], 
        batch_size=config['batch_size'],
        validation_data=(X['val'], y['val']),
        callbacks=[earlyStopping, mcp_save, reduce_lr_loss])

    model.save(model_path)  # Saving the model
    return model



def add_results_emulation(problem, model):
    # Prepare input data - assuming 'input' is already in the correct format
    problem['results_emulation'] = {}
    input_data = problem['input']

    # Check if input_data dimensions match num_vars
    if input_data.shape[1] != problem['num_vars']:
        raise ValueError("Input data dimensions do not match the number of variables")

    # Make predictions
    predictions = model.predict(input_data)

    # Update the problem dictionary with predictions
    problem['results_emulation'] = predictions.flatten()  # Flatten in case predictions are in 2D array

    return problem

 

def predict_single_output(model_path, input_dict, paramset):
    # Load the trained model
    model = tf.keras.models.load_model(model_path)

    # Prepare the input data in the correct order
    input_order = input_dict['names']
    input_values = np.array([input_dict['input'][name] for name in input_order])

    # Reshape input_values for a single prediction
    input_values = input_values.reshape(1, -1)

    # Make the prediction
    prediction = model.predict(input_values)

    # Return the scalar result
    return model_parameters({"ROS":prediction[0, 0]}) 