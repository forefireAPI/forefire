"""
Export a trained emulator to the .ffann file that ForeFire's
ANNPropagationModel loads.

ANNPropagationModel registers every input name in the file as a ForeFire
property and reads it from the fuel table or the front, so the names in the
file have to be ForeFire's, not this package's. The C++ RothermelAndrews2018
model shows the convention: fuel properties keep the name and unit suffix used
here behind a `fuel.` prefix (`fl1h_tac` is `fuel.fl1h_tac`), while wind and
slope come from the front as `normalWind` (m/s) and `slope` (tangent).

The output is the rate of spread in m/s, which is what the training targets
hold: model_parameters stores results in SI.
"""

from .tf_ros_model import load_model_structure, save_model_structure

FRONT_PROPERTIES = {"wind": "normalWind", "slope_tan": "slope"}


def forefire_name(name):
    return FRONT_PROPERTIES.get(name, "fuel." + name)


def export_ffann(model, path, input_names):
    """Write `model` to `path` with the ForeFire names of `input_names`."""
    names = [forefire_name(n) for n in input_names]
    save_model_structure(model, path, input_names=names, output_names=["ROS"])
    return names


def check_ffann(model, path, inputs, rtol=1e-4):
    """Reload `path` and compare its predictions with `model` on `inputs`.

    Returns the largest absolute difference. The file stores float32, so
    the two agree to rounding, not bit for bit.
    """
    import numpy as np

    reloaded, _, _ = load_model_structure(path)
    expected = model.predict(inputs, verbose=0).ravel()
    got = reloaded.predict(inputs, verbose=0).ravel()
    diff = float(np.max(np.abs(expected - got)))
    scale = float(np.max(np.abs(expected))) or 1.0
    if diff > rtol * scale:
        raise ValueError(f"{path} does not reproduce the model: max diff {diff}")
    return diff
