.. _custom-cpp-models:

Writing a model in C++
======================

Propagation models live in ``src/propagation/`` and flux models in ``src/flux/``. How a model gets its inputs is in :doc:`core_concepts`. Both kinds inherit from ``ForeFireModel``, through ``PropagationModel`` or ``FluxModel``. A new model is one ``.cpp`` file.

In its constructor, a model asks for the inputs it needs with ``registerProperty("<name>")``. The call returns an index into the ``double* valueOf`` array, which the ``DataBroker`` fills before calling ``getSpeed(valueOf)`` or ``getValue(valueOf, bt, et, at)``.

.. list-table:: Properties available to ``registerProperty``
   :header-rows: 1
   :widths: 28 72

   * - Name
     - Value
   * - ``fuel.<Column>``
     - Any column of the fuel table, e.g. ``fuel.Rhod``, ``fuel.sd``, ``fuel.Md``, ``fuel.DeltaH`` (:doc:`fuels_and_models`).
   * - ``altitude``
     - Terrain elevation (m).
   * - ``slope``
     - Terrain slope :math:`\tan\alpha` along the outward normal of the node (propagation models).
   * - ``windU``, ``windV``
     - West–east and south–north wind components (m/s).
   * - ``normalWind``
     - Wind projected on the outward normal, :math:`\vec{u} \cdot \vec{n}` (m/s, propagation models).
   * - ``frontDepth``
     - Depth of the burning front along the normal (m). Needs ``frontDepthComputation=1``.
   * - ``frontCurvature``
     - Front curvature :math:`\kappa`. Needs ``curvatureComputation=1``.

As an example, take :math:`R = A \frac{\max(0, \vec{u}\cdot\vec{n})}{1 + M_d}`. Start from a copy of ``src/propagation/SamplePropagationModel.cpp``:

.. code-block:: cpp

   #include "../PropagationModel.h"
   #include "../FireDomain.h"

   namespace libforefire {

   class MyWindMoistureModel : public PropagationModel {
       static const std::string name;
       static int isInitialized;

       size_t normalWind;
       size_t Md;
       double coeffA;

   public:
       MyWindMoistureModel(const int& mindex = 0, DataBroker* db = 0)
           : PropagationModel(mindex, db) {
           // 1. Register environmental and fuel properties
           normalWind = registerProperty("normalWind");
           Md = registerProperty("fuel.Md");

           if (numProperties > 0) properties = new double[numProperties];
           dataBroker->registerPropagationModel(this);

           // 2. Read optional parameter from SimulationParameters
           coeffA = 0.1;
           if (params->isValued("MyWindMoisture.A"))
               coeffA = params->getDouble("MyWindMoisture.A");
       }

       std::string getName() { return name; }

       // 3. Compute Rate of Spread (m/s) from the populated valueOf array
       double getSpeed(double* valueOf) {
           double u = valueOf[normalWind] > 0. ? valueOf[normalWind] : 0.;
           return coeffA * u / (1. + valueOf[Md]);
       }
   };

   const std::string MyWindMoistureModel::name = "MyWindMoisture";
   PropagationModel* getMyWindMoistureModel(const int& idx, DataBroker* db) {
       return new MyWindMoistureModel(idx, db);
   }
   int MyWindMoistureModel::isInitialized =
       FireDomain::registerPropagationModelInstantiator(name, getMyWindMoistureModel);

   } // namespace libforefire

Put the file in ``src/propagation/`` and rebuild with CMake. The model is then available as ``setParameter[propagationModel=MyWindMoisture]``.

Flux models follow the same pattern in ``src/flux/``: inherit from ``FluxModel`` and implement ``getValue(double* valueOf, const double& bt, const double& et, const double& at)``.
