.. _about-statement-of-need:

Statement of Need
=================

**The Problem:** 

Wildfires pose significant environmental, economic, and societal risks globally. Understanding and predicting their behavior requires sophisticated simulation tools capable of handling complex interactions between fire, fuel, weather, and terrain. While various wildfire models exist, challenges remain in areas such as computational performance for large domains, accurate representation of specific physical processes (like fire-atmosphere coupling), and providing flexible platforms for research and operational use.

**ForeFire's Contribution:**

ForeFire was developed by CNRS at the Université de Corse Pascal Paoli to address these needs. It is an open-source (GPLv3) wildfire simulation engine designed for both research and operational applications.

Key motivations and contributions include:

*   **High-Performance Core:** Implemented in C++ with MPI support for efficient parallel execution, enabling large-scale simulations and faster-than-real-time forecasting capabilities essential for operational scenarios.
*   **Physics-Based Modeling:** Incorporates established Rate of Spread (ROS) models (e.g., Rothermel, Balbi) and provides a framework for researchers to implement and test new or custom models.
*   **Fire-Atmosphere Coupling:** Explicitly designed with interfaces (via the core C++ library) to enable two-way coupling with atmospheric models like MesoNH. This allows for capturing critical feedback mechanisms where the fire influences local weather (wind, turbulence) which, in turn, affects fire spread.
*   **Geospatial Data Handling:** Natively utilizes NetCDF for complex landscape and weather inputs, aligning with common scientific data formats.
*   **Flexible Interfaces:** Offers multiple ways to interact with the engine: a command-line interpreter for scripting (`.ff` files) and interactive use, direct C++ library linking for integration into other systems, and Python bindings for scripting and analysis convenience.
*   **Open Source Platform:** Provides a transparent, extensible, and community-driven platform for advancing wildfire science and simulation technology.

**Target Audience:**

ForeFire is intended for:

*   **Wildfire Researchers:** Investigating fire behavior, model development, sensitivity analyses, fire-atmosphere interactions.
*   **Operational Agencies:** Use in forecasting systems (potentially via ensemble simulations), risk assessment, and post-fire analysis.
*   **Students and Educators:** A tool for learning about wildfire dynamics and simulation techniques.