# Wildfire ROS Models Library

![License](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Python Version](https://img.shields.io/badge/Python-3.7%2B-blue.svg)
![GitHub Issues](https://img.shields.io/github/issues/yourusername/wildfire_ROS_models)
![GitHub Stars](https://img.shields.io/github/stars/yourusername/wildfire_ROS_models?style=social)

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
  - [Quick Start](#quick-start)
  - [Example](#example)
- [Directory Structure](#directory-structure)
  - [cases](#cases)
  - [exporter](#exporter)
  - [fuels](#fuels)
  - [models](#models)
  - [tests](#tests)
- [Contributing](#contributing)
- [License](#license)
- [References](#references)
- [Contact](#contact)

## Overview
The **Wildfire ROS Models Library** is a comprehensive Python package designed for simulating the Rate of Spread (ROS) of wildfires. Inspired by various ROS model publications, this library serves as a powerful tool for researchers and practitioners in the field of wildfire management and simulation. It facilitates the intercomparison of different ROS formulations, enhancing the understanding and effectiveness of wildfire behavior predictions.

## Features
- **Diverse ROS Models:** Implements multiple ROS models with references to their original publications.
- **Fuel Data Management:** Organized data and configurations for various fuel types, both model-specific and generic.
- **Case Studies:** Predefined scenarios demonstrating ROS models under different conditions and fuel types.
- **C++ Code Exporter:** Generate C++ code compatible with the ForeFire solver, enabling integration into larger wildfire simulation frameworks.
- **Sensitivity Analysis:** Tools for performing and visualizing sensitivity analyses using Sobol methods.
- **Neural Network Integration:** Emulate ROS models with neural networks for enhanced performance and scalability.
- **Comprehensive Testing:** Example scripts and plots for validating and comparing ROS models.

## Installation

### Prerequisites
- Python 3.7 or higher
- `pip` package manager

### Steps

1. **Clone the Repository:**
   ```bash
   git clone https://github.com/yourusername/wildfire_ROS_models.git
   cd wildfire_ROS_models
   ```

2. **Create a Virtual Environment (Optional but Recommended):**
   ```bash
   python3 -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```

3. **Install Dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

4. **Install the Package:**
   ```bash
   pip install --editable .
   ```

   The `--editable` flag allows you to make changes to the code without reinstalling the package.

## Usage

### Quick Start

After installation, you can use the ROS models in your Python scripts as follows:

```python
from wildfire_ROS_models.models import RothermelAndrews2018
from wildfire_ROS_models.runROS import run_model, plot_results

# Define model parameters
parameters = {
    'wind_speed': 10,  # in mph
    'slope_deg': 0,
    # Add other necessary parameters
}

# Run the model
results = run_model("RothermelAndrews2018", parameters, "wind_speed", range(0, 20, 0.1))

# Plot the results
plot_results(results, 'wind_speed', 'ROS_ftmin')
```

### Example

Refer to the `tests` directory for comprehensive examples and test scripts that demonstrate the application of various ROS models and sensitivity analyses.

```bash
cd tests
python test_models.py
```

## Directory Structure

```
wildfire_ROS_models/
│
├── __init__.py
├── Balbi2020.py
├── Cruz.py
├── RothermelAndrews2018.py
├── fuels_database.py
├── model_set.py
├── neuralNetROS.py
├── runROS.py
├── sensitivity.py
├── interactive_polar_plot.py
├── py_ROS_models_to_forefire_cpp.py
├── utils.py
├── scripts/
│   ├── __init__.py
│   ├── sensitivity_analysis.py
│   └── ... (other scripts)
│
├── tests/
│   ├── __init__.py
│   ├── test_models.py
│   └── test_sensitivity_analysis.py
│
├── docs/
│   └── ... (documentation files)
│
├── README.md
├── LICENSE
├── setup.py
├── requirements.txt
├── MANIFEST.in
└── .gitignore
```

### `cases`
Contains various case studies and scenarios demonstrating the application of ROS models under different conditions and fuel types.

### `exporter`
A module that generates C++ code compatible with the ForeFire solver from Python files, facilitating the integration of ROS models into larger wildfire simulation frameworks.

### `fuels`
Hosts data and configurations related to different types of fuels, including both model-specific and generic data.

### `models`
Includes a variety of ROS models, each referencing their original publication for credibility and ease of comparison.

### `tests`
Contains test scripts and example applications of ROS models, along with sample plots to compare their outputs.

## Contributing

Contributions to the **Wildfire ROS Models Library** are welcome! Whether you're adding new models, improving existing ones, or enhancing documentation, your efforts help advance wildfire research and management.

### How to Contribute

1. **Fork the Repository:**
   Click the "Fork" button at the top right of this repository's GitHub page.

2. **Clone Your Fork:**
   ```bash
   git clone https://github.com/yourusername/wildfire_ROS_models.git
   cd wildfire_ROS_models
   ```

3. **Create a New Branch:**
   ```bash
   git checkout -b feature/your-feature-name
   ```

4. **Make Your Changes:**
   - Add new models by copying existing model Python files and using the same function names.
   - Update documentation as necessary.
   - Ensure all new code includes proper docstrings and follows the project's coding standards.

5. **Commit Your Changes:**
   ```bash
   git add .
   git commit -m "Add [description of your changes]"
   ```

6. **Push to Your Fork:**
   ```bash
   git push origin feature/your-feature-name
   ```

7. **Create a Pull Request:**
   Navigate to your fork on GitHub and click the "Compare & pull request" button. Provide a clear description of your changes and submit the pull request.

### Guidelines

- **Consistency:** Follow the existing coding style and conventions.
- **Documentation:** Ensure all new models and functions are well-documented.
- **Testing:** Add or update tests to cover new functionalities.
- **Commit Messages:** Write clear and descriptive commit messages.

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE). See the [LICENSE](LICENSE) file for details.

## References

- [EcoFire FirebehavioR Data](https://github.com/EcoFire/firebehavioR/tree/master/data)
- [Hourly FFMC](https://github.com/nrcan-cfs-fire/cffdrs-ng)
- Original ROS Models Publications

## Contact

For questions, suggestions, or contributions, please contact:

**Jean-Baptiste Filippi**  
SPE  
[filippi_j@univ-corse.fr](mailto:filippi_j@univ-corse.fr)

---
