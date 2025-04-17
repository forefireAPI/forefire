/**
 * @file BMapLoggerForANNTraining.cpp
 * @brief A Ros model that propagates at the speed of the arrival time map and logs the values, to constitute ROS databases
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "PropagationModel.h"
#include "FireDomain.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>

#include "ANN.h"  // Include the ANN definitions

namespace libforefire {

class BMapLoggerForANNTraining: public PropagationModel {
    static const std::string name;
    static int isInitialized;

    Network annNetwork; // Neural network instance for the model
    std::ofstream csvfile;
public:
    BMapLoggerForANNTraining(const int& = 0, DataBroker* db = nullptr);
    virtual ~BMapLoggerForANNTraining();
    std::string getName();
    double getSpeed(double*);
    double maxSpeed;
    void loadNetwork(const std::string& filename); // Method to load network configuration
};

PropagationModel* getBMapLoggerForANNTrainingModel(const int& = 0, DataBroker* db = nullptr);

} /* namespace libforefire */

using namespace std;
namespace libforefire {

/* name of the model */
const string BMapLoggerForANNTraining::name = "BMapLoggerForANNTraining";
 
/* instantiation */
PropagationModel* getBMapLoggerForANNTrainingModel(const int & mindex, DataBroker* db) {
    return new BMapLoggerForANNTraining(mindex, db);
}

/* registration */
int BMapLoggerForANNTraining::isInitialized =
    FireDomain::registerPropagationModelInstantiator(name, getBMapLoggerForANNTrainingModel);

/* constructor */
BMapLoggerForANNTraining::BMapLoggerForANNTraining(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
    // Load network first to access names
    std::string annPath = params->getParameter("FFANNPropagationModelPath");
    std::string csvPath = params->getParameter("FFBMapLoggerCSVPath");
    maxSpeed = params->getDouble("maxSpeed");
    
    annNetwork.loadFromFile(annPath.c_str());
    csvfile.open(csvPath); 
    properties = new double[annNetwork.inputNames.size() + 1];
    csvfile << "ROS";
    registerProperty("arrival_time_gradient");

    std::cout << "Props: ";
    for (const auto& inputName : annNetwork.inputNames) {
        csvfile << ";"<< inputName ;
        size_t ni = registerProperty(inputName);
        std::cout<<ni<<":"<< inputName<<" ;";
    }
    std::cout << std::endl;
    
    csvfile << std::endl;
    dataBroker->registerPropagationModel(this);
}


BMapLoggerForANNTraining::~BMapLoggerForANNTraining() {
    if (properties) {
        delete[] properties;
    }
        if (csvfile.is_open()) {
        csvfile.close();
    }
}

/* accessor to the name of the model */
string BMapLoggerForANNTraining::getName(){
    return name;
}


double BMapLoggerForANNTraining::getSpeed(double* valueOf) {
    double RosVal = 0.0;
    if (valueOf[0] > 0){
        RosVal = 1.0/valueOf[0]; 
    }else{
        return 0;
    }
    if (RosVal > maxSpeed){
       RosVal = maxSpeed;
    }
    csvfile << RosVal << ";";   
    for (size_t i = 0; i < annNetwork.inputNames.size()-1; ++i) {
        csvfile << valueOf[i+1] << ";";
    }
    csvfile<<valueOf[annNetwork.inputNames.size()] << std::endl;
 
    return RosVal;
}


}