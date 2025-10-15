// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CROSSSIMCOMPUTEARRAY_H
#define _CROSSSIMCOMPUTEARRAY_H

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>
#include <sst/elements/golem/array/computeArray.h>
#include <Python.h>
#include "numpy/arrayobject.h"
#include <iostream>
#include <string>
#include <type_traits>

namespace SST {
namespace Golem {

template<typename T>
class CrossSimComputeArray : public ComputeArray {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(
        CrossSimComputeArray<T>,
        SST::Golem::ComputeArray,
        TimeConverter*,
        Event::HandlerBase*
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"CrossSimJSONParameters", "JSON configuration for CrossSim", "default"}
    )

    CrossSimComputeArray(ComponentId_t id, Params& params,
                         TimeConverter* tc,
                         Event::HandlerBase* handler)
        : ComputeArray(id, params, tc, handler)
    {
        initializePython();
        CrossSimJSON = params.find<std::string>("CrossSimJSONParameters");

        // Configure selfLink
        selfLink = configureSelfLink("Self", tc,
            new Event::Handler2<CrossSimComputeArray,&CrossSimComputeArray::handleSelfEvent>(this));
        selfLink->setDefaultTimeBase(latencyTC);

        // Allocate arrays to hold Python objects
        pyMatrix = new PyObject*[numArrays];
        npMatrix = new PyArrayObject*[numArrays];
        pyArrayIn = new PyObject*[numArrays];
        npArrayIn = new PyArrayObject*[numArrays];
        pyArrayOut = new PyObject*[numArrays];
        npArrayOut = new PyArrayObject*[numArrays];
        cores = new PyObject*[numArrays];
        setMatrixFunction = new PyObject*[numArrays];
        computeMVM = new PyObject*[numArrays];

        // Initialize input/output vector storage
        inputVectors.resize(numArrays);
        outputVectors.resize(numArrays);
        for (uint32_t i = 0; i < numArrays; i++) {
            inputVectors[i].resize(inputArraySize, T());
            outputVectors[i].resize(outputArraySize, T());
        }
    }

    virtual ~CrossSimComputeArray() {
        // Decrement references to Python objects
        for (uint32_t i = 0; i < numArrays; i++) {
            Py_DECREF(pyMatrix[i]);
            Py_DECREF(pyArrayIn[i]);
            Py_DECREF(pyArrayOut[i]);
            Py_DECREF(cores[i]);
            Py_DECREF(setMatrixFunction[i]);
            Py_DECREF(computeMVM[i]);
        }

        // Free our arrays
        delete[] pyMatrix;
        delete[] npMatrix;
        delete[] pyArrayIn;
        delete[] npArrayIn;
        delete[] pyArrayOut;
        delete[] npArrayOut;
        delete[] cores;
        delete[] setMatrixFunction;
        delete[] computeMVM;

        // Decrement other references
        Py_DECREF(crossSim);
        Py_DECREF(paramsConstructor);
        Py_DECREF(AnalogCoreConstructor);
        Py_DECREF(crossSim_params);
        finalizePython();
    }

    virtual void init(unsigned int phase) override {
        if (phase == 0) {
            uint64_t inputSize = inputArraySize;
            uint64_t outputSize = outputArraySize;

            // Prepare NumPy dimension info
            int matrixNumDims = 2;
            npy_intp matrixDims[2] = {
                static_cast<npy_intp>(outputSize),
                static_cast<npy_intp>(inputSize)
            };

            int arrayInNumDims = 1;
            npy_intp arrayInDim[1] = { static_cast<npy_intp>(inputSize) };

            int arrayOutNumDims = 1;
            npy_intp arrayOutDim[1] = { static_cast<npy_intp>(outputSize) };

            int numpyType = getNumpyType();

            // Create Numpy arrays
            for (uint32_t i = 0; i < numArrays; i++) {
                pyMatrix[i] = PyArray_SimpleNew(matrixNumDims, matrixDims, numpyType);
                npMatrix[i] = reinterpret_cast<PyArrayObject*>(pyMatrix[i]);

                pyArrayIn[i] = PyArray_SimpleNew(arrayInNumDims, arrayInDim, numpyType);
                npArrayIn[i] = reinterpret_cast<PyArrayObject*>(pyArrayIn[i]);

                pyArrayOut[i] = PyArray_SimpleNew(arrayOutNumDims, arrayOutDim, numpyType);
                npArrayOut[i] = reinterpret_cast<PyArrayObject*>(pyArrayOut[i]);
            }

            // Import CrossSim (simulator.py) module
            crossSim = PyImport_ImportModule("simulator");
            if (!crossSim) {
                out.fatal(CALL_INFO, -1, "Import CrossSim failed\n");
                PyErr_Print();
            }

            paramsConstructor = PyObject_GetAttrString(crossSim, "CrossSimParameters");
            if (!paramsConstructor) {
                out.fatal(CALL_INFO, -1, "Get CrossSimParameters constructor failed\n");
                PyErr_Print();
            }

            AnalogCoreConstructor = PyObject_GetAttrString(crossSim, "AnalogCore");
            if (!AnalogCoreConstructor) {
                out.fatal(CALL_INFO, -1, "Get AnalogCore constructor failed\n");
                PyErr_Print();
            }

            // Build the CrossSim parameters object
            if (CrossSimJSON.empty()) {
                crossSim_params = PyObject_CallFunction(paramsConstructor, NULL);
            } else {
                PyObject* fromJson = PyObject_GetAttrString(paramsConstructor, "from_json");
                PyObject* jsonArgs = Py_BuildValue("(s)", CrossSimJSON.c_str());
                crossSim_params = PyObject_CallObject(fromJson, jsonArgs);
                Py_DECREF(fromJson);
                Py_DECREF(jsonArgs);
            }
            if (!crossSim_params) {
                out.fatal(CALL_INFO, -1, "Call to CrossSimParameters constructor failed\n");
                PyErr_Print();
            }

            // If the template type is uint64_t, then set params.core.output_dtype = "INT32"
            if constexpr (std::is_same_v<T, int64_t>) {
                // Get the "core" attribute from crossSim_params
                PyObject* core = PyObject_GetAttrString(crossSim_params, "core");
                if (!core) {
                    out.fatal(CALL_INFO, -1, "Get core attribute from CrossSim parameters failed\n");
                    PyErr_Print();
                } else {
                    // Set the output_dtype attribute of core to "INT32"
                    PyObject* dtypeValue = PyUnicode_FromString("INT64");
                    if (PyObject_SetAttrString(core, "output_dtype", dtypeValue) != 0) {
                        out.fatal(CALL_INFO, -1, "Failed to set output_dtype on core\n");
                        PyErr_Print();
                    }
                    Py_DECREF(dtypeValue);
                    Py_DECREF(core);
                }
            }

            // Create the cores
            for (uint32_t i = 0; i < numArrays; i++) {
                cores[i] = PyObject_CallFunctionObjArgs(AnalogCoreConstructor,
                                                        pyMatrix[i],
                                                        crossSim_params,
                                                        NULL);
                if (!cores[i]) {
                    out.fatal(CALL_INFO, -1, "Call to AnalogCore failed\n");
                    PyErr_Print();
                }
                setMatrixFunction[i] = PyObject_GetAttrString(cores[i], "set_matrix");
                if (!setMatrixFunction[i]) {
                    out.fatal(CALL_INFO, -1, "Get core.set_matrix failed\n");
                    PyErr_Print();
                }
                computeMVM[i] = PyObject_GetAttrString(cores[i], "matvec");
                if (!computeMVM[i]) {
                    out.fatal(CALL_INFO, -1, "Get core.matvec failed\n");
                    PyErr_Print();
                }
            }
        }
    }

    virtual void beginComputation(uint32_t arrayID) override {
        SimTime_t latency = getArrayLatency(arrayID);
        ArrayEvent* ev = new ArrayEvent(arrayID);
        selfLink->send(latency, ev);
    }

    virtual void handleSelfEvent(Event* ev) override {
        ArrayEvent* aev = static_cast<ArrayEvent*>(ev);
        uint32_t arrayID = aev->getArrayID();

        compute(arrayID);
        (*tileHandler)(ev);
    }

    virtual void setMatrixItem(int32_t arrayID, int32_t index, double value) override {
        T* data = reinterpret_cast<T*>(PyArray_DATA(npMatrix[arrayID]));
        data[index] = static_cast<T>(value);

        // Once matrix is fully populated, call "set_matrix"
        if (index == inputArraySize * outputArraySize - 1) {
            PyObject* status = PyObject_CallFunctionObjArgs(setMatrixFunction[arrayID],
                                                            npMatrix[arrayID],
                                                            NULL);
            if (!status) {
                out.fatal(CALL_INFO, -1, "Call to core.set_matrix failed\n");
                PyErr_Print();
            }
        }
    }

    virtual void setVectorItem(int32_t arrayID, int32_t index, double value) override {
        T* data = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
        data[index] = static_cast<T>(value);
    }

    virtual void compute(uint32_t arrayID) override {
        // Perform the MVM
        pyArrayOut[arrayID] = PyObject_CallFunctionObjArgs(computeMVM[arrayID],
                                                           npArrayIn[arrayID],
                                                           NULL);
        if (!pyArrayOut[arrayID]) {
            out.fatal(CALL_INFO, -1, "Run MVM Call Failed\n");
            PyErr_Print();
        }
        npArrayOut[arrayID] = reinterpret_cast<PyArrayObject*>(pyArrayOut[arrayID]);

        // Copy output for later retrieval
        int len = PyArray_SIZE(npArrayOut[arrayID]);
        outputVectors[arrayID].resize(len);
        T* outputData = reinterpret_cast<T*>(PyArray_DATA(npArrayOut[arrayID]));
        std::copy(outputData, outputData + len, outputVectors[arrayID].begin());

        // Optional debug printing
        out.verbose(CALL_INFO, 2, 0, "CrossSim MVM on array %u:\n", arrayID);
        T* inputData = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
        T* matrixData = reinterpret_cast<T*>(PyArray_DATA(npMatrix[arrayID]));

        // Print input vector
        for (uint32_t col = 0; col < inputArraySize; col++) {
            printValue(inputData[col]);
        }
        out.verbose(CALL_INFO, 2, 0, "\n\n");

        // Print matrix + output
        for (uint32_t row = 0; row < outputArraySize; row++) {
            for (uint32_t col = 0; col < inputArraySize; col++) {
                printValue(matrixData[row * inputArraySize + col]);
            }
            out.verbose(CALL_INFO, 2, 0, "  ");
            printValue(outputData[row]);
            out.verbose(CALL_INFO, 2, 0, "\n");
        }
        out.verbose(CALL_INFO, 2, 0, "\n\n");
    }

    virtual SimTime_t getArrayLatency(uint32_t arrayID) override {
        return 1;
    }

    virtual void moveOutputToInput(uint32_t srcArrayID, uint32_t destArrayID) override {
        T* src = reinterpret_cast<T*>(PyArray_DATA(npArrayOut[srcArrayID]));
        T* dst = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[destArrayID]));
        std::copy(src, src + outputArraySize, dst);
    }

    virtual void* getInputVector(uint32_t arrayID) override {
        // Convert NumPy array data to std::vector (if needed)
        T* data = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
        int len = PyArray_SIZE(npArrayIn[arrayID]);
        inputVectors[arrayID].resize(len);
        std::copy(data, data + len, inputVectors[arrayID].begin());
        return static_cast<void*>(&inputVectors[arrayID]);
    }

    virtual void* getOutputVector(uint32_t arrayID) override {
        return static_cast<void*>(&outputVectors[arrayID]);
    }

protected:
    std::string CrossSimJSON;

    // Python object references
    PyObject* crossSim            = nullptr;
    PyObject* paramsConstructor   = nullptr;
    PyObject* AnalogCoreConstructor = nullptr;
    PyObject* crossSim_params     = nullptr;

    // Arrays of references
    PyObject** pyMatrix          = nullptr;
    PyArrayObject** npMatrix     = nullptr;
    PyObject** pyArrayIn         = nullptr;
    PyArrayObject** npArrayIn    = nullptr;
    PyObject** pyArrayOut        = nullptr;
    PyArrayObject** npArrayOut   = nullptr;
    PyObject** cores             = nullptr;
    PyObject** setMatrixFunction = nullptr;
    PyObject** computeMVM        = nullptr;

    // Local copy of input/output
    std::vector<std::vector<T>> inputVectors;
    std::vector<std::vector<T>> outputVectors;

    int getNumpyType() {
        if constexpr (std::is_same<T, int64_t>::value) {
            return NPY_INT64;
        } else if constexpr (std::is_same<T, float>::value) {
            return NPY_FLOAT32;
        } else {
            static_assert(!sizeof(T*),
                          "Unsupported data type for CrossSimComputeArray.");
        }
    }

    void printValue(const T& value) {
        if constexpr (std::is_same<T, int64_t>::value) {
            out.verbose(CALL_INFO, 2, 0, "%ld ", value);
        } else if constexpr (std::is_same<T, float>::value) {
            out.verbose(CALL_INFO, 2, 0, "%f ", value);
        }
    }

private:

    static std::atomic<int>& getInstanceCount() {
        static std::atomic<int> count{0};
        return count;
    }

    static std::once_flag& getInitFlag() {
        static std::once_flag flag;
        return flag;
    }

    // This function is called only once, by the first instance constructed.
    static void doPythonInitialization() {
        Py_Initialize();
        import_array1();  // NumPy C-API init
    }

    // Called in the constructor.
    void initializePython() {
        std::call_once(getInitFlag(), doPythonInitialization);
        // Increase instance count.
        ++getInstanceCount();
    }

    // Called in the destructor.
    void finalizePython() {
        // Decrease instance count.
        if (--getInstanceCount() == 0) {
            Py_Finalize();
        }
    }

};

} // namespace Golem
} // namespace SST

#endif /* _CROSSSIMCOMPUTEARRAY_H */
