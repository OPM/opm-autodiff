/*
  Copyright 2015 IRIS AS

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_BLACKOILSOLVENTMODEL_HEADER_INCLUDED
#define OPM_BLACKOILSOLVENTMODEL_HEADER_INCLUDED

#include <opm/autodiff/BlackoilModelBase.hpp>
#include <opm/autodiff/BlackoilModelParameters.hpp>
#include <opm/autodiff/BlackoilSolventState.hpp>
#include <opm/autodiff/WellStateFullyImplicitBlackoilSolvent.hpp>
#include <opm/autodiff/SolventPropsAdFromDeck.hpp>

namespace Opm {

    /// A model implementation for three-phase black oil
    /// with one extra component.
    ///
    ///
    /// It uses automatic differentiation via the class AutoDiffBlock
    /// to simplify assembly of the jacobian matrix.
    template<class Grid>
    class BlackoilSolventModel : public BlackoilModelBase<Grid, BlackoilSolventModel<Grid> >
    {
    public:

        // ---------  Types and enums  ---------

        typedef BlackoilModelBase<Grid, BlackoilSolventModel<Grid> > Base;
        typedef typename Base::ReservoirState ReservoirState;
        typedef typename Base::WellState WellState;
        // The next line requires C++11 support available in g++ 4.7.
        // friend Base;
        friend class BlackoilModelBase<Grid, BlackoilSolventModel<Grid> >;

        /// Construct the model. It will retain references to the
        /// arguments of this functions, and they are expected to
        /// remain in scope for the lifetime of the solver.
        /// \param[in] param               parameters
        /// \param[in] grid                grid data structure
        /// \param[in] fluid               fluid properties
        /// \param[in] geo                 rock properties
        /// \param[in] rock_comp_props     if non-null, rock compressibility properties
        /// \param[in] solvent_props       solvent properties
        /// \param[in] wells               well structure
        /// \param[in] linsolver           linear solver
        /// \param[in] has_disgas          turn on dissolved gas
        /// \param[in] has_vapoil          turn on vaporized oil feature
        /// \param[in] terminal_output     request output to cout/cerr
        /// \param[in] has_solvent         turn on solvent feature
        BlackoilSolventModel(const typename Base::ModelParameters&   param,
                             const Grid&                             grid,
                             const BlackoilPropsAdInterface&         fluid,
                             const DerivedGeology&                   geo,
                             const RockCompressibility*              rock_comp_props,
                             const SolventPropsAdFromDeck&           solvent_props,
                             const Wells*                            wells,
                             const NewtonIterationBlackoilInterface& linsolver,
                             const EclipseStateConstPtr              eclState,
                             const bool                              has_disgas,
                             const bool                              has_vapoil,
                             const bool                              terminal_output,
                             const bool                              has_solvent);

        /// Apply an update to the primary variables, chopped if appropriate.
        /// \param[in]      dx                updates to apply to primary variables
        /// \param[in, out] reservoir_state   reservoir state variables
        /// \param[in, out] well_state        well state variables
        void updateState(const V& dx,
                         ReservoirState& reservoir_state,
                         WellState& well_state);

        /// Compute convergence based on total mass balance (tol_mb) and maximum
        /// residual mass balance (tol_cnv).
        /// \param[in]   dt          timestep length
        /// \param[in]   iteration   current iteration number
        bool getConvergence(const double dt, const int iteration);

        /// Assemble the residual and Jacobian of the nonlinear system.
        /// \param[in]      reservoir_state   reservoir state variables
        /// \param[in, out] well_state        well state variables
        /// \param[in]      initial_assembly  pass true if this is the first call to assemble() in this timestep
        void assemble(const ReservoirState& reservoir_state,
                      WellState& well_state,
                      const bool initial_assembly);


    protected:

        // ---------  Types and enums  ---------

        typedef typename Base::SolutionState SolutionState;
        typedef typename Base::DataBlock DataBlock;
        enum { Solvent = CanonicalVariablePositions::Next };

        // ---------  Data members  ---------
        const bool has_solvent_;
        const int solvent_pos_;
        const SolventPropsAdFromDeck& solvent_props_;

        // Need to declare Base members we want to use here.
        using Base::grid_;
        using Base::fluid_;
        using Base::geo_;
        using Base::rock_comp_props_;
        using Base::wells_;
        using Base::linsolver_;
        using Base::active_;
        using Base::canph_;
        using Base::cells_;
        using Base::ops_;
        using Base::wops_;
        using Base::has_disgas_;
        using Base::has_vapoil_;
        using Base::param_;
        using Base::use_threshold_pressure_;
        using Base::threshold_pressures_by_interior_face_;
        using Base::rq_;
        using Base::phaseCondition_;
        using Base::well_perforation_pressure_diffs_;
        using Base::residual_;
        using Base::terminal_output_;
        using Base::primalVariable_;
        using Base::pvdt_;

        // ---------  Protected methods  ---------

        // Need to declare Base members we want to use here.
        using Base::wellsActive;
        using Base::wells;
        using Base::variableState;
        using Base::computePressures;
        using Base::computeGasPressure;
        using Base::applyThresholdPressures;
        using Base::fluidViscosity;
        using Base::fluidReciprocFVF;
        using Base::fluidDensity;
        using Base::fluidRsSat;
        using Base::fluidRvSat;
        using Base::poroMult;
        using Base::transMult;
        using Base::updatePrimalVariableFromState;
        using Base::updatePhaseCondFromPrimalVariable;
        using Base::dpMaxRel;
        using Base::dsMax;
        using Base::drMaxRel;
        using Base::maxResidualAllowed;

        using Base::updateWellControls;
        using Base::computeWellConnectionPressures;
        using Base::addWellControlEq;
	using Base::blockPattern;

        std::vector<ADDB>
        computeRelPerm(const SolutionState& state) const;

        void
        makeConstantState(SolutionState& state) const;

        std::vector<V>
        variableStateInitials(const ReservoirState& x,
                              const WellState& xw) const;

        std::vector<int>
        variableStateIndices() const;

        SolutionState
        variableStateExtractVars(const ReservoirState& x,
                                 const std::vector<int>& indices,
                                 std::vector<ADDB>& vars) const;

        void
        computeAccum(const SolutionState& state,
                     const int            aix  );

        void
        assembleMassBalanceEq(const SolutionState& state);

        void
        addWellContributionToMassBalanceEq(const std::vector<ADB>& cq_s,
                                           const SolutionState& state,
                                           WellState& xw);

        void
        computeMassFlux(const int               actph ,
                        const V&                transi,
                        const ADDB&             kr    ,
                        const ADDB&             p     ,
                        const SolutionState&    state );

        const std::vector<PhasePresence>
        phaseCondition() const {return this->phaseCondition_;}

        /// \brief Compute the reduction within the convergence check.
        /// \param[in] B     A matrix with MaxNumPhases columns and the same number rows
        ///                  as the number of cells of the grid. B.col(i) contains the values
        ///                  for phase i.
        /// \param[in] tempV A matrix with MaxNumPhases columns and the same number rows
        ///                  as the number of cells of the grid. tempV.col(i) contains the
        ///                   values
        ///                  for phase i.
        /// \param[in] R     A matrix with MaxNumPhases columns and the same number rows
        ///                  as the number of cells of the grid. B.col(i) contains the values
        ///                  for phase i.
        /// \param[out] R_sum An array of size MaxNumPhases where entry i contains the sum
        ///                   of R for the phase i.
        /// \param[out] maxCoeff An array of size MaxNumPhases where entry i contains the
        ///                   maximum of tempV for the phase i.
        /// \param[out] B_avg An array of size MaxNumPhases where entry i contains the average
        ///                   of B for the phase i.
        /// \param[in]  nc    The number of cells of the local grid.
        /// \return The total pore volume over all cells.
        double
        convergenceReduction(const Eigen::Array<double, Eigen::Dynamic, MaxNumPhases+1>& B,
                             const Eigen::Array<double, Eigen::Dynamic, MaxNumPhases+1>& tempV,
                             const Eigen::Array<double, Eigen::Dynamic, MaxNumPhases+1>& R,
                             std::array<double,MaxNumPhases+1>& R_sum,
                             std::array<double,MaxNumPhases+1>& maxCoeff,
                             std::array<double,MaxNumPhases+1>& B_avg,
                             std::vector<double>& maxNormWell,
                             int nc,
                             int nw) const;

    };



    /// Need to include concentration in our state variables, otherwise all is as
    /// the default blackoil model.
    struct BlackoilSolventSolutionState : public DefaultBlackoilSolutionState
    {
        explicit BlackoilSolventSolutionState(const int np)
            : DefaultBlackoilSolutionState(np),
              solvent_saturation( ADDB::null())
        {
        }
        ADDB solvent_saturation;
    };



    /// Providing types by template specialisation of ModelTraits for BlackoilSolventModel.
    template <class Grid>
    struct ModelTraits< BlackoilSolventModel<Grid> >
    {
        typedef BlackoilSolventState ReservoirState;
        typedef WellStateFullyImplicitBlackoilSolvent WellState;
        typedef BlackoilModelParameters ModelParameters;
        typedef BlackoilSolventSolutionState SolutionState;
    };

} // namespace Opm

#include "BlackoilSolventModel_impl.hpp"

#endif // OPM_BLACKOILSOLVENTMODEL_HEADER_INCLUDED
