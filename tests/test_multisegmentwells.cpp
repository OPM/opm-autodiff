/*
  Copyright 2016 SINTEF ICT, Applied Mathematics.
  Copyright 2016 Statoil ASA.

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


#include <config.h>

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE MultisegmentWellsTest

#include <vector>
#include <memory>
#include <array>


#include <opm/common/utility/platform_dependent/disable_warnings.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <opm/common/utility/platform_dependent/reenable_warnings.h>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

#include <opm/core/grid.h>
#include <opm/core/props/satfunc/SaturationPropsFromDeck.hpp>
#include <opm/core/utility/Units.hpp>
#include <opm/core/wells/WellsManager.hpp>
#include <opm/core/wells.h>
#include <opm/core/wells/DynamicListEconLimited.hpp>

#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/autodiff/GridHelpers.hpp>
#include <opm/autodiff/createGlobalCellArray.hpp>
#include <opm/autodiff/GridInit.hpp>

#include <opm/autodiff/BlackoilPropsAdFromDeck.hpp>
#include <opm/autodiff/MultisegmentWells.hpp>



struct SetupMSW {

    using Grid = UnstructuredGrid;
    using GridInit = Opm::GridInit<Grid>;
    using FluidProps = Opm::BlackoilPropsAdFromDeck;
    using MaterialLawManager = FluidProps::MaterialLawManager;

    SetupMSW()
    {
        Opm::ParseContext parse_context;
        Opm::ParserPtr parser(new Opm::Parser());
        Opm::DeckConstPtr deck = parser->parseFile("msw.data", parse_context);
        Opm::EclipseStateConstPtr ecl_state(new Opm::EclipseState(*deck , parse_context));

        // Create grid.
        const std::vector<double>& porv =
                            ecl_state->get3DProperties().getDoubleGridProperty("PORV").getData();

        std::unique_ptr<GridInit> grid_init(new GridInit(ecl_state, porv));
        const Grid& grid = grid_init->grid();

        // Create material law manager.
        std::vector<int> compressed_to_cartesianIdx;
        Opm::createGlobalCellArray(grid, compressed_to_cartesianIdx);

        std::shared_ptr<MaterialLawManager> material_law_manager(new MaterialLawManager());
        material_law_manager->initFromDeck(deck, ecl_state, compressed_to_cartesianIdx);

        std::unique_ptr<FluidProps> fluidprops(new FluidProps(deck, ecl_state, material_law_manager, grid));

        const size_t current_timestep = 0;

        // dummy_dynamic_list_econ_lmited
        const Opm::DynamicListEconLimited dummy_dynamic_list;

        // Create wells.
        Opm::WellsManager wells_manager(ecl_state,
                                        current_timestep,
                                        Opm::UgGridHelpers::numCells(grid),
                                        Opm::UgGridHelpers::globalCell(grid),
                                        Opm::UgGridHelpers::cartDims(grid),
                                        Opm::UgGridHelpers::dimensions(grid),
                                        Opm::UgGridHelpers::cell2Faces(grid),
                                        Opm::UgGridHelpers::beginFaceCentroids(grid),
                                        fluidprops->permeability(),
                                        dummy_dynamic_list,
                                        false);

        const Wells* wells = wells_manager.c_wells();
        const auto wells_ecl = ecl_state->getSchedule()->getWells(current_timestep);

        ms_wells.reset(new Opm::MultisegmentWells(wells, wells_ecl, current_timestep));
    };

    std::shared_ptr<const Opm::MultisegmentWells> ms_wells;
};

// number of wells for this case
const int nw = 2;
// number of segments for this case
const int nseg = 7;
// number of perforations for this case
const int nperf = 8;

BOOST_AUTO_TEST_CASE(testOperators)
{
    SetupMSW msw_setup;
    const std::shared_ptr<const Opm::MultisegmentWells>& ms_wells = msw_setup.ms_wells;

    const Opm::MultisegmentWells::MultisegmentWellOps& wops_ms = ms_wells->wellOps();
    BOOST_CHECK_EQUAL(true, wops_ms.has_multisegment_wells);
    BOOST_CHECK_EQUAL(nperf, wops_ms.well_cells.size());
    BOOST_CHECK_EQUAL(nperf, wops_ms.conn_trans_factors.size());
    BOOST_CHECK_EQUAL(nseg, wops_ms.s2s_outlet.rows());
    BOOST_CHECK_EQUAL(nseg, wops_ms.s2s_inlets.cols());
    BOOST_CHECK_EQUAL(nw, wops_ms.s2w.rows());
    BOOST_CHECK_EQUAL(nseg, wops_ms.s2w.cols());
    BOOST_CHECK_EQUAL(nseg, wops_ms.topseg2w.cols());
    BOOST_CHECK_EQUAL(nperf, wops_ms.s2p.rows());
    BOOST_CHECK_EQUAL(nseg, wops_ms.p2s.rows());
    BOOST_CHECK_EQUAL(nperf, wops_ms.w2p.rows());
    BOOST_CHECK_EQUAL(nw, wops_ms.w2p.cols());
    BOOST_CHECK_EQUAL(nperf, wops_ms.p2w.cols());
}


BOOST_AUTO_TEST_CASE(testStructure)
{
    SetupMSW msw_setup;
    const std::shared_ptr<const Opm::MultisegmentWells>& ms_wells = msw_setup.ms_wells;

    BOOST_CHECK_EQUAL(3, ms_wells->numPhases());
    BOOST_CHECK_EQUAL(nseg, ms_wells->numSegment());
    BOOST_CHECK_EQUAL(nperf, ms_wells->numPerf());

    BOOST_CHECK_EQUAL(nw, ms_wells->topWellSegments().size());
    BOOST_CHECK_EQUAL(nw, ms_wells->msWells().size());
    BOOST_CHECK_EQUAL(0, ms_wells->topWellSegments()[0]);
    BOOST_CHECK_EQUAL(1, ms_wells->topWellSegments()[1]);
}
