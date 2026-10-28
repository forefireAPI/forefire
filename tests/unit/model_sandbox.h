/**
 * @file model_sandbox.h
 * @brief Minimal scaffolding to exercise a single model outside a simulation.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 */

#ifndef FF_TESTS_MODEL_SANDBOX_H
#define FF_TESTS_MODEL_SANDBOX_H

#include <map>
#include <string>
#include <vector>

namespace libforefire {
class FireDomain;
class FluxModel;
class ForeFireModel;
class PropagationModel;
class SimulationParameters;
}

namespace ff_test {

/** A fire domain that exists only to hand out models.
 *
 * Propagation and flux models are not constructible on their own: their
 * constructors call back into the DataBroker, which calls back into the
 * FireDomain. So a unit test still needs a domain — but only an empty one,
 * with no landscape, no fuel map and no fire. Everything a model reads from
 * the simulation arrives through the `double*` it is handed, which is what
 * makes `getSpeed` and `getValue` testable in isolation.
 *
 * Parameters are process-global: every SimulationParameters method reads and
 * writes SimulationParameters::GetInstance() whatever instance it is called
 * on, so there is no per-domain parameter set to hand out. A test that sets
 * `Iso.speed` or `burningDuration` would therefore change the result of the
 * next one. To stop that, the sandbox snapshots the whole parameter map on
 * construction and restores it on destruction — see ~ModelSandbox.
 */
class ModelSandbox {
public:
    /** Builds the domain. Parameters set after this point are still seen by
     *  models, because models read them in their constructor — but the fuel
     *  table is read here, so it has to be passed in. */
    explicit ModelSandbox(const std::string& fuelTable = standardFuelTable());

    ~ModelSandbox();

    /** The global parameter set, restored to its prior contents when this
     *  sandbox dies. Set values here *before* asking for a model: that is when
     *  the model reads its coefficients. */
    libforefire::SimulationParameters* parameters() const { return params; }

    /** The domain the sandbox built. */
    libforefire::FireDomain* fireDomain() const { return domain; }

    /** Instantiates a registered propagation model, or returns 0 if the name
     *  is not in the registry. Each call gets a fresh model index. */
    libforefire::PropagationModel* propagation(const std::string& name);

    /** Instantiates a registered flux model, or returns 0 if unknown. */
    libforefire::FluxModel* flux(const std::string& name);

    /** The fuel table used by default: the columns of the Balbi/Rothermel
     *  family, as in tests/runff/fuels.csv. Models asking for fuel properties
     *  outside this set warn on construction; that is not a failure, it just
     *  means the table does not describe them. */
    static const std::string& standardFuelTable();

private:
    ModelSandbox(const ModelSandbox&);
    ModelSandbox& operator=(const ModelSandbox&);

    libforefire::SimulationParameters* params;
    libforefire::FireDomain* domain;
    std::map<std::string, std::string> savedParameters;
    int nextPropagationIndex;
    int nextFluxIndex;
};

/** The property array a model reads, addressed by name instead of by index.
 *
 * A model's properties are numbered in the order its constructor calls
 * registerProperty, so `valueOf[7]` means whatever the eighth call happened to
 * register. Tests that hardcode those numbers break the moment someone
 * reorders the constructor — silently, by testing a different quantity. This
 * looks the index up from the model's own wantedProperties instead.
 */
class Inputs {
public:
    explicit Inputs(libforefire::ForeFireModel*);

    /** Sets one property. Aborts the test if the model never asked for it,
     *  which is the honest outcome: the test is describing a model that does
     *  not exist. */
    Inputs& set(const std::string& property, double value);

    /** True if the model registered this property. */
    bool has(const std::string& property) const;

    /** The array to hand to getSpeed/getValue. Null for a model with no
     *  properties, which is what those models are handed in production. */
    double* data();

private:
    size_t indexOf(const std::string& property) const;

    libforefire::ForeFireModel* model;
    std::vector<double> values;
};

/** Fills every property a model asks for: fuel 1 of the standard table, still
 *  air on flat ground, and a front one metre deep with no curvature.
 *  Individual tests then override whatever they are actually varying.
 *
 * Returns false if the model reads something this helper has no value for,
 * so a test can leave that model alone rather than run it on zeroes.
 */
bool fillStandardConditions(Inputs&, libforefire::ForeFireModel*);

} /* namespace ff_test */

#endif /* FF_TESTS_MODEL_SANDBOX_H */
