/*! \file Peridigm.hpp */

//@HEADER
// ************************************************************************
//
//                             Peridigm
//                 Copyright (2011) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions?
// David J. Littlewood   djlittl@sandia.gov
// John A. Mitchell      jamitch@sandia.gov
// Michael L. Parks      mlparks@sandia.gov
// Stewart A. Silling    sasilli@sandia.gov
//
// ************************************************************************
//@HEADER

#ifndef PERIDIGM_HPP
#define PERIDIGM_HPP

#include <vector>
#include <set>

#include <Epetra_MpiComm.h>
#include <Epetra_SerialComm.h>
#include <Teuchos_FancyOStream.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>
#include <Epetra_FECrsMatrix.h>

#include "contact/Peridigm_ContactModel.hpp"
#include "materials/Peridigm_Material.hpp"
#include "Peridigm_AbstractDiscretization.hpp"
#include "Peridigm_ModelEvaluator.hpp"
#include "Peridigm_DataManager.hpp"
#include "Peridigm_SerialMatrix.hpp"
#include "Peridigm_OutputManager.hpp"
#include "Peridigm_ComputeManager.hpp"

#include "PdGridData.h"

namespace PeridigmNS {

  class Peridigm {

  public:

    //! Constructor
    Peridigm(const Teuchos::RCP<const Epetra_Comm>& comm,
             const Teuchos::RCP<Teuchos::ParameterList>& params);

    //! Destructor
    ~Peridigm(){};

    //! Instantiate material objects
    void instantiateMaterials();

    //! Initialize material objects
    void initializeMaterials();

    //! Initialize discretization and maps
    void initializeDiscretization(Teuchos::RCP<AbstractDiscretization> peridigmDisc);

    //! Initialize dataManager
    void initializeDataManager(Teuchos::RCP<AbstractDiscretization> peridigmDisc);

    //! Apply boundary conditions
    void applyInitialVelocities();

    //! Initialize contact
    void initializeContact();

    //! Initialize the workset
    void initializeWorkset();

    //! Initialize the compute manager
    void initializeComputeManager();

    //! Initialize the output manager
    void initializeOutputManager();

    //! Main routine to drive time integration
    void execute();

    //! Main routine to drive problem solution with explicit time integration
    void executeExplicit();

    //! Main routine to drive problem solution with implicit time integration
    void executeImplicit();

    //! Allocate memory for non-zeros in global Jacobian
    void allocateJacobian();

    //! Apply kinematic boundary conditions: for Jacobain, zero out rows and columns and put one on diagonal; for residual put displacement increment corresponding to bc.
    void applyKinematicBC(double loadIncrement, Teuchos::RCP<Epetra_Vector> vec, Teuchos::RCP<Epetra_FECrsMatrix> mat);

    //! Compute the residual for quasi-statics
    double computeResidual();

    //! Synchronize data in DataManager across processes (needed before call to OutputManager::write() )
    void synchDataManager();

    //! Rebalance the mesh
    void rebalance();

    //! Compute a parallel decomposion based on the current configuration
    PdGridData currentConfigurationDecomp();

    //! Create a rebalanced bond map
    Teuchos::RCP<Epetra_BlockMap> createRebalancedBondMap(Teuchos::RCP<Epetra_BlockMap> rebalancedOneDimensionalMap,
                                                          Teuchos::RCP<const Epetra_Import> oneDimensionalMapToRebalancedOneDimensionalMapImporter);

    //! Create a global ID neighbor list in a rebalanced partitioning
    Teuchos::RCP<Epetra_Vector> createRebalancedNeighborGlobalIDList(Teuchos::RCP<Epetra_BlockMap> rebalancedBondMap,
                                                                     Teuchos::RCP<const Epetra_Import> bondMapToRebalancedBondMapImporter);

    //! Create a rebalanced NeighborhoodData object
    Teuchos::RCP<PeridigmNS::NeighborhoodData> createRebalancedNeighborhoodData(Teuchos::RCP<Epetra_BlockMap> rebalancedOneDimensionalMap,
                                                                                Teuchos::RCP<Epetra_BlockMap> rebalancedOneDimensionalOverlapMap,
                                                                                Teuchos::RCP<Epetra_BlockMap> rebalancedBondMap,
                                                                                Teuchos::RCP<Epetra_Vector> rebalancedNeighborGlobalIDs);

    //! Fill the contact neighbor information in rebalancedDecomp and populate contactNeighborsGlobalIDs and offProcesorContactIDs
    void contactSearch(Teuchos::RCP<const Epetra_BlockMap> rebalancedOneDimensionalMap,
                       Teuchos::RCP<const Epetra_BlockMap> rebalancedBondMap,
                       Teuchos::RCP<const Epetra_Vector> rebalancedNeighborGlobalIDs,
                       PdGridData& rebalancedDecomp,
                       Teuchos::RCP< std::map<int, std::vector<int> > > contactNeighborGlobalIDs,
                       Teuchos::RCP< std::set<int> > offProcessorContactIDs);

    //! Create a rebalanced NeighborhoodData object for contact
    Teuchos::RCP<PeridigmNS::NeighborhoodData> createRebalancedContactNeighborhoodData(Teuchos::RCP<std::map<int, std::vector<int> > > contactNeighborGlobalIDs,
                                                                                       Teuchos::RCP<const Epetra_BlockMap> rebalancedOneDimensionalMap,
                                                                                       Teuchos::RCP<const Epetra_BlockMap> rebalancedOneDimensionalOverlapMap);

    //! @name Acessors for maps 
    //@{ 
    Teuchos::RCP<const Epetra_BlockMap> getOneDimensionalMap() { return oneDimensionalMap; }
    Teuchos::RCP<const Epetra_BlockMap> getOneDimensionalOverlapMap() { return oneDimensionalOverlapMap; }
    Teuchos::RCP<const Epetra_BlockMap> getThreeDimensionalMap() { return threeDimensionalMap; }
    Teuchos::RCP<const Epetra_BlockMap> getThreeDimensionalOverlapMap() { return threeDimensionalOverlapMap; }
    Teuchos::RCP<const Epetra_BlockMap> getBondMap() { return bondMap; }
    //@}

    //! @name Accessors for main solver-level vectors
    //@{ 
    Teuchos::RCP<const Epetra_Vector> getX() { return x; }
    Teuchos::RCP<const Epetra_Vector> getU() { return u; }
    Teuchos::RCP<const Epetra_Vector> getY() { return y; }
    Teuchos::RCP<const Epetra_Vector> getV() { return v; }
    Teuchos::RCP<const Epetra_Vector> getA() { return a; }
    Teuchos::RCP<const Epetra_Vector> getForce() { return force; }
    Teuchos::RCP<const Epetra_Vector> getContactForce() { return contactForce; }
    Teuchos::RCP<const Epetra_Vector> getDeltaU() { return deltaU; }
    Teuchos::RCP<const Epetra_Vector> getResidual() { return residual; }
    //@}

    //! @name Accessors for neighborhood data
    //@{
    Teuchos::RCP<const PeridigmNS::NeighborhoodData> getNeighborhoodData() { return neighborhoodData; }
    Teuchos::RCP<const PeridigmNS::NeighborhoodData> getContactNeighborhoodData() { return contactNeighborhoodData; }
    //@}

    //! Accessor for DataManager
    Teuchos::RCP<PeridigmNS::DataManager> getDataManager() { return dataManager; }

    //! Return list of field specs used by Peridigm object
    std::vector<Field_NS::FieldSpec> getFieldSpecs();

  private:

    //! Parameterlist of entire input deck
    Teuchos::RCP<Teuchos::ParameterList> peridigmParams;

    //! Epetra communicator established by Peridigm_Factory
    Teuchos::RCP<const Epetra_Comm> peridigmComm;

    //! Output stream
    Teuchos::RCP<Teuchos::FancyOStream> out;

    //! Maps for scalar, vector, and bond data
    Teuchos::RCP<const Epetra_BlockMap> oneDimensionalMap;
    Teuchos::RCP<const Epetra_BlockMap> oneDimensionalOverlapMap;
    Teuchos::RCP<const Epetra_BlockMap> threeDimensionalMap;
    Teuchos::RCP<const Epetra_BlockMap> threeDimensionalOverlapMap;
    Teuchos::RCP<const Epetra_BlockMap> bondMap;

    //! Importers and exporters from global to overlapped vectors
    Teuchos::RCP<const Epetra_Import> oneDimensionalMapToOneDimensionalOverlapMapImporter;
    Teuchos::RCP<const Epetra_Import> threeDimensionalMapToThreeDimensionalOverlapMapImporter;

    //! Rebalance flag
    bool analysisHasRebalance;

    //! Rebalance frequency
    int rebalanceFrequency;

    //! Contact flag
    bool analysisHasContact;

    //! Contact search frequency
    int contactRebalanceFrequency;

    //! Contact search radius
    double contactSearchRadius;

    //! Material models
    //! \todo Use Teuchos::ArrayRCP to store materials?
    Teuchos::RCP< std::vector< Teuchos::RCP<const PeridigmNS::Material> > > materialModels;

    //! Contact models
    Teuchos::RCP< std::vector<Teuchos::RCP<const PeridigmNS::ContactModel> > > contactModels;

    //! Compute manager
    Teuchos::RCP<PeridigmNS::ComputeManager> computeManager;

    //! Data manager
    Teuchos::RCP<PeridigmNS::DataManager> dataManager;

    //! Mothership multivector that contains all the global vectors (x, u, y, v, a, force, etc.)
    Teuchos::RCP<Epetra_MultiVector> mothership;

    //! Overlap Jacobian; filled by each processor and then assembled into the mothership Jacobian;
    Teuchos::RCP<PeridigmNS::SerialMatrix> overlapJacobian;

    //! Global vector for initial positions
    Teuchos::RCP<Epetra_Vector> x;

    //! Global vector for displacement
    Teuchos::RCP<Epetra_Vector> u;

    //! Global vector for current position
    Teuchos::RCP<Epetra_Vector> y;

    //! Global vector for velocity
    Teuchos::RCP<Epetra_Vector> v;

    //! Global vector for acceleration
    Teuchos::RCP<Epetra_Vector> a;

    //! Global vector for force
    Teuchos::RCP<Epetra_Vector> force;

    //! Global vector for contact force (used only in simulations with contact)
    Teuchos::RCP<Epetra_Vector> contactForce;

    //! Global vector for delta u (used only in implicit time integration)
    Teuchos::RCP<Epetra_Vector> deltaU;

    //! Global vector for residual (used only in implicit time integration)
    Teuchos::RCP<Epetra_Vector> residual;

    //! Map for global tangent matrix (note, must be an Epetra_Map, not an Epetra_BlockMap)
    Teuchos::RCP<Epetra_Map> tangentMap;

    //! Global tangent matrix
    Teuchos::RCP<Epetra_FECrsMatrix> tangent;

    //! List of neighbors for all locally-owned nodes
    Teuchos::RCP<PeridigmNS::NeighborhoodData> neighborhoodData;

    //! List of potential contact neighbors for all locally-owned nodes
    Teuchos::RCP<PeridigmNS::NeighborhoodData> contactNeighborhoodData;

    //! Workset that is passed to the modelEvaluator
    Teuchos::RCP<PHAL::Workset> workset;

    //! The peridigm model evaluator
    Teuchos::RCP<PeridigmNS::ModelEvaluator> modelEvaluator;

    //! The peridigm output manager
    Teuchos::RCP<PeridigmNS::OutputManager> outputManager;

    //! Description of force state data used by output manager
    Teuchos::RCP<Teuchos::ParameterList> forceStateDesc;

    //! BLAS for local-only vector updates (BLAS-1)
    Epetra_BLAS blas;
  };
}

#endif // PERIDIGM_HPP
