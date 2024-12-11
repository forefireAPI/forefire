/*

Copyright (C) 2024 ForeFire Team, SPE, Universit� de Corse.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 US

 */

#ifndef ANN_H
#define ANN_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory> // For std::unique_ptr
#include <cstring>
#include <sstream>
#include <stdexcept>

// Activation functions
inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

inline float relu(float x) {
    return std::max(0.0f, x);
}

inline float linear(float x) {
    return x;
}

// Define the tanh activation function
inline float tanh_activation(float x) {
    return std::tanh(x);
}

// Activation function lookup
inline float (*getActivationFunction(const std::string& name))(float) {
    if (name == "RELU") return relu;
    if (name == "SIGM") return sigmoid;
    if (name == "TANH") return tanh_activation; // Added tanh
    if (name == "LINE") return linear;
    return nullptr;
}

// Layer structure
struct BaseLayer {
    virtual ~BaseLayer() {}

    // Pure virtual function for computing the output of the layer
    virtual std::vector<float> feedforward(const std::vector<float>& inputs) = 0;
};
struct DenseLayer : public BaseLayer {
    std::vector<float> neurons;
    std::vector<std::vector<float>> weights;
    std::vector<float> biases;
    float (*activation)(float);

    // Constructor
    DenseLayer(int inputSize, int outputSize, float (*actFunc)(float))
        : neurons(outputSize, 0.0f), weights(outputSize, std::vector<float>(inputSize)),
          biases(outputSize), activation(actFunc) {}

    void loadWeightsAndBiases(const std::vector<float>& weightData, const std::vector<float>& biasData) {
        for (size_t i = 0; i < weights.size(); ++i) {
            for (size_t j = 0; j < weights[i].size(); ++j) {
                weights[i][j] = weightData[j * weights.size() + i];
            }
        }
        std::copy(biasData.begin(), biasData.end(), biases.begin());
    }

    std::vector<float> feedforward(const std::vector<float>& inputs) override {
        std::vector<float> output(neurons.size(), 0.0f);
        for (size_t i = 0; i < neurons.size(); ++i) {
            float neuronOutput = biases[i];
            for (size_t j = 0; j < inputs.size(); ++j) {
                neuronOutput += weights[i][j] * inputs[j];
            }
            output[i] = activation(neuronOutput);
        }
        return output;
    }
};
struct NormalizationLayer : public BaseLayer {
    std::vector<float> mean;
    std::vector<float> variance;

    // Constructor
    NormalizationLayer(const std::vector<float>& meanData, const std::vector<float>& varianceData)
        : mean(meanData), variance(varianceData) {}

    std::vector<float> feedforward(const std::vector<float>& inputs) override {
        std::vector<float> output(inputs.size());
        for (size_t i = 0; i < inputs.size(); ++i) {
            output[i] = (inputs[i] - mean[i]) / sqrt(variance[i] + 1e-10); // Safe division
        }
        return output;
    }
};
// Network structure
struct Network {
    std::vector<std::unique_ptr<BaseLayer>> layers; // Correct declaration
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;

    std::vector<std::string> splitNames(const std::string& names) {
        std::vector<std::string> result;
        std::istringstream iss(names);
        std::string token;
        while (getline(iss, token, ',')) {
            result.push_back(token);
        }
        return result;
    }

   
    void loadFromFile(const char* filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error(std::string("Failed to open network structure file: ") + filename);
        }

        char header[9] = {0};
        int numLayers;
        file.read(header, 8);
        file.read(reinterpret_cast<char*>(&numLayers), sizeof(int));

        if (std::string(header) != "FFANN001") {
            throw std::runtime_error("Invalid file format.");
        }
        for (int i = 0; i < numLayers; ++i) {
            char activation[5] = {0};
            int width, height;
            file.read(activation, 4);
            file.read(reinterpret_cast<char*>(&width), sizeof(int));
            file.read(reinterpret_cast<char*>(&height), sizeof(int));
            std::cout << "layer "<<activation<<" "<<width<<" "<<height<<std::endl;
      
            if (strcmp(activation, "NORM") == 0) {
                // Normalization layer
                std::vector<float> meanData(width), varianceData(width);
                file.read(reinterpret_cast<char*>(meanData.data()), width * sizeof(float));
                file.read(reinterpret_cast<char*>(varianceData.data()), width * sizeof(float));
                layers.push_back(std::make_unique<NormalizationLayer>(meanData, varianceData));
            } else {
                // Dense layer
                float (*actFunc)(float) = getActivationFunction(std::string(activation));
                if (!actFunc) {
                    throw std::runtime_error("Unsupported activation function.");
                }
                DenseLayer *layer = new DenseLayer(width, height, actFunc);
                std::vector<float> weightData(height * width);
                std::vector<float> biasData(height);
                file.read(reinterpret_cast<char*>(weightData.data()), height * width * sizeof(float));
                file.read(reinterpret_cast<char*>(biasData.data()), height * sizeof(float));

                layer->loadWeightsAndBiases(weightData, biasData);
                layers.push_back(std::unique_ptr<BaseLayer>(layer));
                std::cout << "adding dense "<<std::endl;
            }
        }

        // Reading input and output names
        int input_names_length, output_names_length;
        std::string nameBuffer;
        file.read(reinterpret_cast<char*>(&input_names_length), sizeof(int));
        nameBuffer.resize(input_names_length);
        file.read(&nameBuffer[0], input_names_length);
        inputNames = splitNames(nameBuffer);

        file.read(reinterpret_cast<char*>(&output_names_length), sizeof(int));
        nameBuffer.resize(output_names_length);
        file.read(&nameBuffer[0], output_names_length);
        outputNames = splitNames(nameBuffer);

        std::cout << "Network loaded successfully. Input layers: " << inputNames.size() 
                << ", Output layers: " << outputNames.size() << std::endl;

        printLayerInfo();
    }
    void printLayerInfo() {
        std::cout << toString() << std::endl;
    }

    std::string toString() const {
        std::ostringstream ss;
        int layerCount = 0; // Track the number of layers

        for (const auto& layer : layers) {
            if (auto denseLayer = dynamic_cast<DenseLayer*>(layer.get())) { // Check if it's a DenseLayer
                std::string act="None";
                if (denseLayer->activation == relu) act = "ReLU";
                if (denseLayer->activation == sigmoid) act = "Sigmoid";
                if (denseLayer->activation == linear) act = "Linear";
                if (denseLayer->activation == tanh_activation) act = "TanH";
                ss << "Dense Layer: Input Size = " << denseLayer->weights[0].size()
                << ", Output Size = " << denseLayer->neurons.size()
                << ", Activation Function = " << act << "\n";
            } else if (auto normLayer = dynamic_cast<NormalizationLayer*>(layer.get())) { // Check if it's a NormalizationLayer
                ss << "Normalization Layer: Mean Size = " << normLayer->mean.size()
                << ", Variance Size = " << normLayer->variance.size() << "\n";
            } else {
                ss << "Unknown Layer Type\n";
            }

            layerCount++;
        }

        ss << "Total layers: " << layerCount << "\n";
        ss << "\nInputs:";
        for (const auto& name : inputNames) {
            ss << name << ";";
        }
        ss << "\nOutputs:";
        for (const auto& name : outputNames) {
            ss << name << "";
        }
        ss << "\n";
        return ss.str();
    }

    std::vector<float> processInput(const std::vector<float>& input) {
        std::vector<float> result = input;
        for (auto& layer : layers) {
            result = layer->feedforward(result);
        }
        return result;
    }
   

};

#endif // ANN_H

