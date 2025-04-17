/**
 * @file ANNPropagationModel.cpp
 * @brief A fire ROS model using a feed forward ANN.
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

class ANNPropagationModel: public PropagationModel {
    /*! name the model */
    static const std::string name;
    /*! boolean for initialization */
    static int isInitialized;

    Network annNetwork; // Neural network instance for the model

public:
    ANNPropagationModel(const int& = 0, DataBroker* db = nullptr);
    virtual ~ANNPropagationModel();
    std::string getName();
    double getSpeed(double*);

    void loadNetwork(const std::string& filename); // Method to load network configuration
};

PropagationModel* getANNPropagationModelModel(const int& = 0, DataBroker* db = nullptr);

} /* namespace libforefire */

using namespace std;
namespace libforefire {

/* name of the model */
const string ANNPropagationModel::name = "ANNPropagationModel";

/* instantiation */
PropagationModel* getANNPropagationModelModel(const int & mindex, DataBroker* db) {
    return new ANNPropagationModel(mindex, db);
}

/* registration */
int ANNPropagationModel::isInitialized =
    FireDomain::registerPropagationModelInstantiator(name, getANNPropagationModelModel);

/* constructor */
ANNPropagationModel::ANNPropagationModel(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
    std::string annPath = params->getParameter("FFANNPropagationModelPath");
    annNetwork.loadFromFile(annPath.c_str());
    properties = new double[annNetwork.inputNames.size()];
     std::cout << "Props: ";

    for (size_t i = 0; i < annNetwork.inputNames.size(); ++i) {
        
       size_t ni =   registerProperty(annNetwork.inputNames[i]);
        std::cout<<ni<<":"<< annNetwork.inputNames[i]<<" ;";
    }
    std::cout << endl;
    dataBroker->registerPropagationModel(this);
}



ANNPropagationModel::~ANNPropagationModel() {
    if (properties) {
        delete[] properties;
    }
}

/* accessor to the name of the model */
string ANNPropagationModel::getName(){
    return name;
}

double ANNPropagationModel::getSpeed(double* valueOf) {
    std::vector<float> inputs(numProperties);
 
    for (size_t i = 0; i < numProperties; ++i) {
        inputs[i] = static_cast<float>(valueOf[i]);
    }

    std::vector<float> outputs = annNetwork.processInput(inputs);
    double result = static_cast<double>(outputs[0]);

    /* Print inputs followed by result
    std::cout << outputs[0]<< ";"  ;
    for (size_t i = 0; i < inputs.size(); ++i) {
        std::cout << inputs[i];
        if (i != inputs.size() - 1) {
            std::cout << ";";
        }
    }
    std::cout << std::endl;
   */
  if (result < 0)
    {
        return 0.0;
    }
  if (result > 2){
    return 2;
  }

    return result ;
   
}

}