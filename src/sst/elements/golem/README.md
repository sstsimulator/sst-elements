# Project Golem

This project includes emulated/CrossSim arrays, Vanadis-RoCC interfaces, and testing for analog matrix-vector multiplication (MVM) applications.

## Directory Structure

### `golem/array`
Contains specifications for various compute arrays.

- **ComputeArray**
  - **MVMComputeArray**
    - **MVMFloatArray**
    - **MVMIntArray**
  - **CrossSimComputeArray**
    - **CrossSimFloatArray**

### `golem/rocc`
Contains analog specifications for RoCC.

- **RoCCAnalog**
  - **RoCCAnalogFloat**
  - **RoCCAnalogInt**

### `golem/tests`
Includes tests for different MVM applications. Below is the structure of planned and completed tests.
- **basic_mvm**: Contains basic MVM tests on three compute arrays:
  - **crossSimArray**
    - `analog_library_example`: MVM example using CrossSimArray and analog library.
    - `intrinsics_example`: MVM example using CrossSimArray.
  - **floatArray**
    - `analog_library_example`: MVM example using MVMFloatArray and analog library.
    - `intrinsics_example`: MVM example using MVMFloatArray.
  - **intArray**
    - `analog_library_example_int`: MVM example using MVMIntArray and analog library.
    - `analog_library_example_float`: MVM example using MVMIntArray with quantized float values.
    - `intrinsics_example`: MVM example using MVMIntArray.
- **multi_array**: Contains a "ping-pong" tests between multiple compute arrays. The tests usee the three compute arrays:
  - **crossSimArray**
    - `analog_library_example`: Ping-Pong MVM example using CrossSimArray and analog library.
    - `intrinsics_example`: Ping-Pong MVM example using CrossSimArray.
  - **floatArray**
    - `analog_library_example`: Ping-Pong MVM example using MVMFloatArray and analog library.
    - `intrinsics_example`: Ping-Pong MVM example using MVMFloatArray.
  - **intArray**
    - `analog_library_example_int`: Ping-Pong MVM example using MVMIntArray and analog library.
    - `intrinsics_example`: Ping-Pong MVM example using MVMIntArray.
    - `analog_library_example_float`: Planned but not yet complete 

- **multi_core_mvm**: Planned but not yet completed.
- **multi_array_multi_core_mvm**: Planned but not yet completed.

### Files

- **Makefile.am**: Build file for subcomponents.
- **golem.cc**: Main file for including subcomponents.
