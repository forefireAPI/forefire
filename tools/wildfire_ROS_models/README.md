# wildfire_ROS_models

Python implementations of rate of spread (ROS) models, used to compare their
formulations, run sensitivity analyses and emulate them with neural networks.
It lives in the ForeFire repository under `tools/` and is not part of the
simulator build.

## Contents

- `wildfire_ROS_models/`: the package.
  - `Rothermel1972.py`, `RothermelAndrews2018.py`, `Balbi2020.py`, `Cruz.py`: the models.
  - `fuels_database.py`, `model_set.py`: fuel data and parameter sets.
  - `runROS.py`: `run_model` sweeps one input over a range, `plot_results` plots the sweeps.
  - `sensitivity.py`: Sobol sensitivity analysis (SALib).
  - `neuralNetROS.py`, `tf_ros_model.py`, `tf_helpers.py`: neural-network emulation.
  - `py_ROS_models_to_forefire_cpp.py`: translates a Python model function into a ForeFire C++ class.
  - `interactive_polar_plot.py`, `utils.py`: helpers.
- `tests/`: `test_models.py` (model sweeps and plots) and `test_sensitivity_analysis.py`.
- `sobol_sensitivity_analysis.py`, `scripts/sobol_sensitivity_analysis.py`, `train_nn.py`: analysis and training scripts.
- `docs/wildfireROS.tex`: notes on the models.

## Install

```bash
cd tools/wildfire_ROS_models
pip install -r requirements.txt
pip install --editable .
```

## Example

Balbi2020 on pine needles at three fuel loads, sweeping wind speed (from
`tests/test_models.py`):

```python
import numpy as np
from wildfire_ROS_models import fuels_database as fdb
from wildfire_ROS_models.model_set import model_parameters
from wildfire_ROS_models.runROS import run_model, plot_results

pn = fdb.load_csv(fdb.pineNeedlesBalbi2020_csv)[0] + model_parameters(
    {"wind_mps": 0, "slope_deg": 0}
)
fuels = []
for load in [0.08, 0.3, 0.8]:
    fm = model_parameters(pn.get_set())
    fm.fl1h_kgm2 = load
    fm.CODE = f"Pine Needles load {load}"
    fuels.append(fm)

runs = [run_model("Balbi2020", fm, "wind_mps", np.arange(0, 12, 1)) for fm in fuels]
plot_results(runs, "wind_mps", "ROS_mps")
```

Run the tests with `pytest tests/test_models.py`.

## License

See `LICENSE`.

## Contact

Jean-Baptiste Filippi, Université de Corse – SPE, filippi_j@univ-corse.fr
