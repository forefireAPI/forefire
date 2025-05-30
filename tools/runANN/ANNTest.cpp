#include "../../src/propagation/ANN.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>  // For std::pow

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <network_file> <data_file> [print_results]\n";
        return 1;
    }
    bool printResults = (argc > 3);  // Check for the optional third argument


    Network network;
    network.loadFromFile(argv[1]);  // Load network configuration from the file

    std::cout << "Network Configuration:\n" << network.toString() << std::endl;

    std::ifstream file(argv[2]);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << argv[2] << std::endl;
        return 1;
    }

    std::string line;
    std::getline(file, line);  // Skip header

    std::vector<std::vector<float>> inputs;
    std::vector<float> expectedOutputs;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        std::vector<float> inputRow;
        float ros;
        bool firstColumn = true;

        while (getline(ss, cell, ';')) {
            if (firstColumn) {
                ros = std::stof(cell);
                expectedOutputs.push_back(ros);
                firstColumn = false;
            } else {
                inputRow.push_back(std::stof(cell));
            }
        }
        inputs.push_back(inputRow);
    }
    file.close();

    double totalLoss = 0.0;

    // Timing the feedforward process
    auto start = std::chrono::high_resolution_clock::now();

    // Process each test input through the network and print the results
    for (size_t i = 0; i < inputs.size(); ++i) {
        auto outputs = network.processInput(inputs[i]);
        if (printResults) {
            std::cout << "[";
            for (auto val : inputs[i]) std::cout << val << " ";
            std::cout << "] result: " << outputs[0] << " observed " << expectedOutputs[i] << std::endl;
        }

        double loss = std::pow(outputs[0] - expectedOutputs[i], 2);
        totalLoss += loss;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // Compute Root Mean Squared Error
    double rmse = std::sqrt(totalLoss / inputs.size());

    // Output timing and error results
    std::cout << "Time taken for processing " << inputs.size() << " inputs: " << elapsed.count() << " seconds\n";
    std::cout << "Total Root Mean Squared Error: " << rmse << std::endl;


    return 0;
}
