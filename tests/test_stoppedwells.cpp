/*
  Copyright 2014 IRIS
  
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
#include <chrono>

#define BOOST_TEST_MODULE StoppedWellsTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>


using namespace Opm;

BOOST_AUTO_TEST_CASE(TestStoppedWells)
{
    const std::string filename = "wells_stopped.data";
    Opm::Parser parser;
    Opm::Deck deck(parser.parseFile(filename));
    Opm::EclipseState eclipseState(deck);
    const auto& grid = eclipseState.getInputGrid();
    const TableManager table ( deck );
    const Eclipse3DProperties eclipseProperties ( deck , table, grid);
    const Opm::Runspec runspec (deck);
    const Schedule sched(deck, grid, eclipseProperties, runspec);

    const std::vector<double> pressure = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    // Both wells are open in the first schedule step
    {
        auto wells = sched.getWells2(0);
        BOOST_CHECK(wells[0].getStatus() == Opm::Well2::Status::OPEN);
        BOOST_CHECK(wells[1].getStatus() == Opm::Well2::Status::OPEN);
    }


    // The injector is stopped
    {
        auto wells = sched.getWells2(1);
        BOOST_CHECK(wells[0].getStatus() == Opm::Well2::Status::STOP);
        BOOST_CHECK(wells[1].getStatus() == Opm::Well2::Status::OPEN);
    }
}
