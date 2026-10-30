# setup.py

from setuptools import setup, find_packages

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="wildfire_ROS_models",  # Replace with your desired package name
    version="0.1.0",
    author="Your Name",
    author_email="filippi_j@univ-corse.fr",
    description="A package for wildfire ROS model development, benchmarking and testing",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/forefireAPI/wildfire_ROS_models/",  # Replace with your repo URL
    packages=["wildfire_ROS_models"],
    package_dir={"wildfire_ROS_models": "wildfire_ROS_models"},
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",  # Choose your license
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.7",
    install_requires=[
        "matplotlib>=3.5.0",
        "numpy>=1.21.0",
        "SALib>=1.5.3",
        "scikit-learn>=1.0.2",
        "tensorflow>=2.0.0",  # Added TensorFlow
        # Add other dependencies here
    ],
    entry_points={
        "console_scripts": [
            "sensitivity_analysis=wildfire_ROS_models.scripts.sensitivity_analysis:main",
        ],
    },
    include_package_data=True,
)
