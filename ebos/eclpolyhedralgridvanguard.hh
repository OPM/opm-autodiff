// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 * \copydoc Opm::EclPolyhedralGridVanguard
 */
#ifndef EWOMS_ECL_POLYHEDRAL_GRID_VANGUARD_HH
#define EWOMS_ECL_POLYHEDRAL_GRID_VANGUARD_HH

#include <memory>

#include "eclbasevanguard.hh"
#include "ecltransmissibility.hh"

#include <opm/grid/polyhedralgrid.hh>

namespace Opm {
template <class TypeTag>
class EclPolyhedralGridVanguard;
}

BEGIN_PROPERTIES

NEW_TYPE_TAG(EclPolyhedralGridVanguard, INHERITS_FROM(EclBaseVanguard));

// declare the properties
SET_TYPE_PROP(EclPolyhedralGridVanguard, Vanguard, Opm::EclPolyhedralGridVanguard<TypeTag>);
SET_TYPE_PROP(EclPolyhedralGridVanguard, Grid, Dune::PolyhedralGrid<3, 3>);
SET_TYPE_PROP(EclPolyhedralGridVanguard, GlobalIOGrid, typename GET_PROP_TYPE(TypeTag, Grid));

END_PROPERTIES

namespace Opm {

/*!
 * \ingroup EclBlackOilSimulator
 *
 * \brief Helper class for grid instantiation of ECL file-format using problems.
 *
 * This class uses Dune::PolyhedralGrid as the simulation grid.
 */
template <class TypeTag>
class EclPolyhedralGridVanguard : public EclBaseVanguard<TypeTag>
{
    friend class EclBaseVanguard<TypeTag>;
    typedef EclBaseVanguard<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Simulator) Simulator;

public:
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, GlobalIOGrid) GlobalIOGrid;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    typedef EclTransmissibility<TypeTag>  EclTransmissibilityType;

private:
    typedef std::unique_ptr< Grid > GridPointer;
    typedef std::unique_ptr< GlobalIOGrid > GlobalIOGridPointer;
    typedef Dune::CartesianIndexMapper<Grid> CartesianIndexMapper;
    typedef std::unique_ptr< CartesianIndexMapper > CartesianIndexMapperPointer;

public:
    EclPolyhedralGridVanguard(Simulator& simulator)
        : EclBaseVanguard<TypeTag>(simulator)
    {
        this->callImplementationInit();
    }

    /*!
     * \brief Return a reference to the simulation grid.
     */
    Grid& grid()
    { return *grid_; }

    /*!
     * \brief Return a reference to the simulation grid.
     */
    const Grid& grid() const
    { return *grid_; }

    /*!
     * \brief Returns a reference to the grid which should be used by the EQUIL
     *        initialization code.
     *
     * The EQUIL keyword is used to specify the initial condition of the reservoir in
     * hydrostatic equilibrium. Since the code which does this is not accepting arbitrary
     * DUNE grids (the code is part of the opm-core module), this is not necessarily the
     * same as the grid which is used for the actual simulation.
     */
    const GlobalIOGrid& globalIOGrid() const
    { return *grid_; }

    /*!
     * \brief Indicates that the initial condition has been computed and the memory used
     *        by the EQUIL grid can be released.
     *
     * Depending on the implementation, subsequent accesses to the EQUIL grid lead to
     * crashes.
     */
    void releaseGlobalIOGrid()
    { /* do nothing: The GlobalIOGrid grid is the simulation grid! */ }

    /*!
     * \brief Distribute the simulation grid over multiple processes
     *
     * (For parallel simulation runs.)
     */
    void loadBalance()
    { /* do nothing: PolyhedralGrid is not parallel! */ }

    /*!
     * \brief Free the memory occupied by the global transmissibility object.
     *
     * After writing the initial solution, this array should not be necessary anymore.
     */
    void releaseGlobalTransmissibilities()
    {
        globalTrans_.reset();
    }

    /*!
     * \brief Returns the object which maps a global element index of the simulation grid
     *        to the corresponding element index of the logically Cartesian index.
     */
    const CartesianIndexMapper& cartesianIndexMapper() const
    { return *cartesianIndexMapper_; }

    /*!
     * \brief Returns mapper from compressed to cartesian indices for the EQUIL grid
     *
     * Since PolyhedralGrid is not parallel, that's always the same as
     * cartesianIndexMapper().
     */
    const CartesianIndexMapper& globalIOCartesianIndexMapper() const
    { return *cartesianIndexMapper_; }

    std::unordered_set<std::string> defunctWellNames() const
    { return defunctWellNames_; }

    const EclTransmissibilityType& globalTransmissibility() const
    {
        assert( globalTrans_ != nullptr );
        return *globalTrans_;
    }

protected:
    void createGrids_()
    {
        const auto& gridProps = this->eclState().get3DProperties();
        const std::vector<double>& porv = gridProps.getDoubleGridProperty("PORV").getData();

        grid_.reset( new Grid(this->deck(), porv) );
        cartesianIndexMapper_.reset( new CartesianIndexMapper(*grid_) );
    }

    void filterConnections_()
    {
        // not handling the removal of completions for this type of grid yet.
    }

    GridPointer grid_;
    CartesianIndexMapperPointer cartesianIndexMapper_;

    std::unique_ptr<EclTransmissibilityType> globalTrans_;
    std::unordered_set<std::string> defunctWellNames_;
};

} // namespace Opm

#endif
