/**
 * @file model_sandbox.cpp
 * @brief Implementation of the unit-test scaffolding.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 */

#include "model_sandbox.h"

#include "FireDomain.h"
#include "FluxModel.h"
#include "ForeFireModel.h"
#include "PropagationModel.h"
#include "SimulationParameters.h"

#include "doctest/doctest.h"

#include <map>

using libforefire::FireDomain;
using libforefire::FluxModel;
using libforefire::ForeFireModel;
using libforefire::PropagationModel;
using libforefire::SimulationParameters;

namespace ff_test {

const std::string& ModelSandbox::standardFuelTable() {
    // Fuels 0 and 1 of tests/runff/fuels.csv. Fuel 0 has a zero fuel depth,
    // which several models treat as "nothing to burn"; fuel 1 is the one the
    // numerical tests use.
    static const std::string table =
        "Index;Rhod;Rhol;Md;Ml;sd;sl;e;Sigmad;Sigmal;stoch;RhoA;Ta;Tau0;"
        "Deltah;DeltaH;Cp;Cpa;Ti;X0;r00;Blai;me\n"
        "0;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;"
        "18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3\n"
        "1;563.0;522.0;0.1;1.0;6099.0;7273.0;0.24;0.764;0.352;8.3;1.0;300;70000;"
        "18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3\n";
    return table;
}

namespace {

/** The value SimulationParameters reports for a key it does not hold.
 *
 * The sentinel itself is a private static, so it is read back out of the class
 * rather than duplicated here: any key that cannot plausibly have been set
 * returns it. Assigning it to a key makes isValued() false again, which is the
 * closest thing to erasing one that the interface offers. */
const std::string& unsetValue() {
    static const std::string sentinel =
        SimulationParameters::GetInstance()->getParameter(
            "ff_test.no.such.parameter");
    return sentinel;
}

} /* anonymous namespace */

ModelSandbox::ModelSandbox(const std::string& fuelTable)
    : params(SimulationParameters::GetInstance())
    , domain(0)
    , nextPropagationIndex(0)
    , nextFluxIndex(0) {
    // Parameters are process-global on this branch: SimulationParameters'
    // methods all operate on GetInstance() whatever instance they are called
    // on, so a private set would be ignored. Snapshot instead, and put it back
    // in the destructor, so that a test setting burningDuration to zero cannot
    // reach the test that runs after it.
    const std::vector<std::string> keys = params->getAllKeys();
    for (size_t i = 0; i < keys.size(); i++) {
        savedParameters[keys[i]] = params->getParameter(keys[i]);
    }

    // The DataBroker parses the fuel table while the domain is being built, so
    // this has to be in place before the domain exists.
    params->setParameter("fuelsTable", fuelTable);

    libforefire::FFPoint sw(0., 0., 0.);
    libforefire::FFPoint ne(1000., 1000., 0.);
    domain = new FireDomain(0., sw, ne);
}

ModelSandbox::~ModelSandbox() {
    // The domain owns its broker, which owns the models it handed out.
    delete domain;

    // Restore what was there, and blank whatever this sandbox introduced.
    const std::vector<std::string> keys = params->getAllKeys();
    for (size_t i = 0; i < keys.size(); i++) {
        std::map<std::string, std::string>::const_iterator saved
            = savedParameters.find(keys[i]);
        params->setParameter(keys[i],
                             saved != savedParameters.end() ? saved->second
                                                            : unsetValue());
    }
}

PropagationModel* ModelSandbox::propagation(const std::string& name) {
    return domain->propModelInstanciation(nextPropagationIndex++, name);
}

FluxModel* ModelSandbox::flux(const std::string& name) {
    return domain->fluxModelInstanciation(nextFluxIndex++, name);
}

Inputs::Inputs(ForeFireModel* m)
    : model(m)
    , values(m != 0 ? m->numProperties : 0, 0.) {
}

size_t Inputs::indexOf(const std::string& property) const {
    for (size_t i = 0; i < model->wantedProperties.size(); i++) {
        if (model->wantedProperties[i] == property) return i;
    }
    return model->wantedProperties.size();
}

bool Inputs::has(const std::string& property) const {
    return model != 0 && indexOf(property) < model->wantedProperties.size();
}

Inputs& Inputs::set(const std::string& property, double value) {
    const size_t i = indexOf(property);
    REQUIRE_MESSAGE(i < values.size(),
                    "model " << model->getName() << " does not read '"
                             << property << "'");
    values[i] = value;
    return *this;
}

double* Inputs::data() {
    return values.empty() ? 0 : &values[0];
}

namespace {

/** Fuel 1 of the standard table, plus the ambient quantities a model may ask
 *  for on top of the fuel. Anything not listed here makes fillStandardConditions
 *  give up rather than leave a property at zero. */
const std::map<std::string, double>& standardValues() {
    static std::map<std::string, double> v;
    if (!v.empty()) return v;
    v["fuel.Rhod"] = 563.0;
    v["fuel.Rhol"] = 522.0;
    v["fuel.Md"] = 0.1;
    v["fuel.Ml"] = 1.0;
    v["fuel.sd"] = 6099.0;
    v["fuel.sl"] = 7273.0;
    v["fuel.e"] = 0.24;
    v["fuel.Sigmad"] = 0.764;
    v["fuel.Sigmal"] = 0.352;
    v["fuel.stoch"] = 8.3;
    v["fuel.RhoA"] = 1.0;
    v["fuel.Ta"] = 300.0;
    v["fuel.Tau0"] = 70000.0;
    v["fuel.Deltah"] = 18169000.0;
    v["fuel.DeltaH"] = 18167000.0;
    v["fuel.Cp"] = 1800.0;
    v["fuel.Cpa"] = 1000.0;
    v["fuel.Ti"] = 600.0;
    v["fuel.X0"] = 0.3;
    v["fuel.r00"] = 2.5e-05;
    v["fuel.Blai"] = 4.0;
    v["fuel.me"] = 0.3;
    // Ambient conditions and front geometry, not fuel properties.
    v["slope"] = 0.0;
    v["normalWind"] = 0.0;
    v["moisture"] = 0.1;
    v["deadMoisture"] = 0.1;
    v["temperature"] = 300.0;
    v["frontDepth"] = 1.0;
    v["frontCurvature"] = 0.0;
    return v;
}

} /* anonymous namespace */

bool fillStandardConditions(Inputs& inputs, ForeFireModel* model) {
    const std::map<std::string, double>& known = standardValues();
    for (size_t i = 0; i < model->wantedProperties.size(); i++) {
        const std::string& property = model->wantedProperties[i];
        std::map<std::string, double>::const_iterator it = known.find(property);
        if (it == known.end()) return false;
        inputs.set(property, it->second);
    }
    return true;
}

} /* namespace ff_test */
