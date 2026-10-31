# tests/test_sensitivity_analysis.py

import unittest
from unittest.mock import patch, MagicMock
from wildfire_ROS_models.sensitivity import (
    generate_problem_set,
    plot_sobol_indices,
    sobol_analysis,
    main,
)
from wildfire_ROS_models.tf_ros_model import load_model_structure, add_results_emulation

import matplotlib.pyplot as plt


class TestSensitivityAnalysis(unittest.TestCase):
    @patch("wildfire_ROS_models.sensitivity.generate_problem_set")
    @patch("wildfire_ROS_models.sensitivity.sobol_analysis")
    @patch("wildfire_ROS_models.sensitivity.plot_sobol_indices")
    @patch("wildfire_ROS_models.neuralNetROS.load_model_structure")
    @patch("wildfire_ROS_models.neuralNetROS.tf.keras.models.load_model")
    @patch("wildfire_ROS_models.neuralNetROS.add_results_emulation")
    def test_main(
        self,
        mock_add_results_emulation,
        mock_load_model,
        mock_load_model_structure,
        mock_plot_sobol_indices,
        mock_sobol_analysis,
        mock_generate_problem_set,
    ):
        """Test the main function of the sensitivity_analysis script."""

        # Mock arguments
        class Args:
            root = "/fake/root"
            target_ros_model = "RothermelAndrews2018"
            nn_model_path = "/fake/path/nn_model.h5"
            n_samples = 1000
            epochs = 100
            batch_size = 128
            lr = 0.001
            l1_reg_coeff = 0.01
            val_prop = 0.2
            patience = 10
            overwrite = False
            plot = False

        args = Args()

        # Setup mock returns
        mock_generate_problem_set.return_value = {
            "model_name": "RothermelAndrews2018",
            "input": [],
            "results": [],
        }
        mock_sobol_analysis.return_value = (
            {"S1": [0.1]},
            ["param1"],
            [0],
            "RothermelAndrews2018",
        )

        # Mock model loading
        mock_load_model.return_value = MagicMock()

        # Mock add_results_emulation
        mock_add_results_emulation.return_value = {"results_emulation": []}
        mock_sobol_analysis.return_value = (
            {"S1": [0.2]},
            ["param1"],
            [0],
            "RothermelAndrews2018",
        )

        # Run main
       # main(args)

        # Assertions
        mock_generate_problem_set.assert_called_once_with(
            "RothermelAndrews2018", N=1000
        )
        mock_sobol_analysis.assert_called()
        mock_plot_sobol_indices.assert_called()
        mock_load_model.assert_called_once_with("/fake/path/nn_model.h5")
        mock_add_results_emulation.assert_called()

    def test_plot_sobol_indices(self):
        """Test the plotting function."""
        # Mock data
        Si = {"S1": [0.1, 0.2], "ST": [0.3, 0.4]}
        params = ["param1", "param2"]
        y_pos = [0, 1]
        model_name = "TestModel"

        try:
            plot_sobol_indices(Si, params, y_pos, model_name)
        except Exception as e:
            self.fail(f"plot_sobol_indices raised an exception: {e}")


if __name__ == "__main__":
    unittest.main()
