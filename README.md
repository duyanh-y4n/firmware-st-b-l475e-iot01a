# Speech command interface

- This project is a fork of this project from Edge Impulse: https://github.com/edgeimpulse/firmware-st-b-l475e-iot01ao
- To upload dataset and train AI speech model with Edge Impulse, please refer to the note book "ei-audio-dataset-curation.ipynb" this project: https://github.com/ShawnHymel/ei-keyword-spotting
  (a copy of this repository is also included in this folder)

## Getting Started

- please install mbedos toolchain as follow:

  - install python-poetry (https://python-poetry.org/docs/#installation)
  - activate python virtual environment

  ```bash
  poetry run shell

  # expected python v3.7 if the virtual environment is activated successfully
  python --version
  ```

  - install mbedos

  ```bash
  pip install mbed-cli
  ```

## Install mbedos SDK

```bash
poetry run mbed deploy
```

## build firmware

```bash
poetry run make debug_build
```

- more information on uploadind and debugging program, please refer to EI_README.md

## Training and compiling new model
+ The model used in this work is published at https://studio.edgeimpulse.com/public/20675/latest
+ Please go to Deployment section of looking at the official documentation from Edge Impulse for more details.

## code documentation

Code documentation is generated automatically with doxygen. Open file doxygen/html/index.html to access documentation.

