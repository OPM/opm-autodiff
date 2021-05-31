/*
  Copyright Equinor ASA 2021

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

#ifndef OPM_SEGMENTSTATE_HEADER_INCLUDED
#define OPM_SEGMENTSTATE_HEADER_INCLUDED

#include <vector>

namespace Opm
{

class WellSegments;
class WellConnections;


class SegmentState
{
public:
    SegmentState() = default;
    SegmentState(int num_phases, const WellSegments& segments);
    double pressure_drop(std::size_t index) const;
    bool empty() const;

    std::vector<double> rates;
    std::vector<double> pressure;
    std::vector<double> pressure_drop_friction;
    std::vector<double> pressure_drop_hydrostatic;
    std::vector<double> pressure_drop_accel;
};

}

#endif
