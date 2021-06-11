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
/**
 * \file
 *
 * \copydoc Opm::EclTracerModel
 */
#ifndef EWOMS_ECL_GENERIC_TRACER_MODEL_HH
#define EWOMS_ECL_GENERIC_TRACER_MODEL_HH

#include <opm/grid/common/CartesianIndexMapper.hpp>

#include <opm/models/blackoil/blackoilmodel.hh>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <dune/istl/bcrsmatrix.hh>

#include <dune/common/version.hh>

#include <string>
#include <vector>
#include <iostream>

namespace Opm {

class EclipseState;

template<class Grid, class GridView, class DofMapper, class Stencil, class Scalar>
class EclGenericTracerModel {
public:
    using TracerMatrix = Dune::BCRSMatrix<Dune::FieldMatrix<Scalar, 1, 1>>;
    using TracerVector = Dune::BlockVector<Dune::FieldVector<Scalar,1>>;
    using CartesianIndexMapper = Dune::CartesianIndexMapper<Grid>;

    /*!
     * \brief Return the number of tracers considered by the tracerModel.
     */
    int numTracers() const
    { return tracerNames_.size(); }

    /*!
     * \brief Return the tracer name
     */
    const std::string& tracerName(int tracerIdx) const;

    /*!
     * \brief Return the tracer concentration for tracer index and global DofIdx
     */
    Scalar tracerConcentration(int tracerIdx, int globalDofIdx) const;

protected:
    EclGenericTracerModel(const GridView& gridView,
                          const EclipseState& eclState,
                          const CartesianIndexMapper& cartMapper,
                          const DofMapper& dofMapper);

    /*!
     * \brief Initialize all internal data structures needed by the tracer module
     */
    void doInit(bool enabled,
                size_t numGridDof,
                size_t gasPhaseIdx,
                size_t oilPhaseIdx,
                size_t waterPhaseIdx);

    bool linearSolve_(const TracerMatrix& M, TracerVector& x, TracerVector& b);

    const GridView& gridView_;
    const EclipseState& eclState_;
    const CartesianIndexMapper& cartMapper_;
    const DofMapper& dofMapper_;

    std::vector<std::string> tracerNames_;
    std::vector<int> tracerPhaseIdx_;
    std::vector<Dune::BlockVector<Dune::FieldVector<Scalar, 1>>> tracerConcentration_;
    std::vector<Dune::BlockVector<Dune::FieldVector<Scalar, 1>>> tracerConcentrationInitial_;
    TracerMatrix *tracerMatrix_;
    TracerVector tracerResidual_;
    std::vector<int> cartToGlobal_;
    std::vector<Dune::BlockVector<Dune::FieldVector<Scalar, 1>>> storageOfTimeIndex1_;
};

} // namespace Opm

#endif
