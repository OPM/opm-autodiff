/*
  Copyright 2012 SINTEF ICT, Applied Mathematics.

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

#ifndef OPM_WELLSTATE_HEADER_INCLUDED
#define OPM_WELLSTATE_HEADER_INCLUDED

#include <opm/core/props/BlackoilPhases.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <opm/simulators/wells/PerforationData.hpp>
#include <opm/simulators/wells/WellContainer.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Opm
{

class ParallelWellInfo;
class SummaryState;

/// The state of a set of wells.
class WellState
{
public:
    using mapentry_t = std::array<int, 3>;
    using WellMapType = std::map<std::string, mapentry_t>;

    explicit WellState(const PhaseUsage& pu) :
        phase_usage_(pu)
    {}

    WellState() = default;
    WellState(const WellState& rhs)  = default;

    virtual ~WellState() = default;

    WellState& operator=(const WellState& rhs) = default;

    /// Allocate and initialize if wells is non-null.
    /// Also tries to give useful initial values to the bhp() and
    /// wellRates() fields, depending on controls.  The
    /// perfRates() field is filled with zero, and perfPress()
    /// with -1e100.
    void init(const std::vector<double>& cellPressures,
              const std::vector<Well>& wells_ecl,
              const std::vector<ParallelWellInfo*>& parallel_well_info,
              const std::vector<std::vector<PerforationData>>& well_perf_data,
              const SummaryState& summary_state);



    /// One rate per well and phase.
    const WellContainer<std::vector<double>>& wellRates() const { return wellrates_; }
    std::vector<double>& wellRates(std::size_t well_index) { return wellrates_[well_index]; }
    const std::vector<double>& wellRates(std::size_t well_index) const { return wellrates_[well_index]; }

    /// One rate per well connection.
    std::vector<double>& perfRates(std::size_t well_index) { return this->perfrates_[well_index]; }
    const std::vector<double>& perfRates(std::size_t well_index) const { return this->perfrates_[well_index]; }
    std::vector<double>& perfRates(const std::string& wname) { return this->perfrates_[wname]; }
    const std::vector<double>& perfRates(const std::string& wname) const { return this->perfrates_[wname]; }

    /// One pressure per well connection.
    std::vector<double>& perfPress(std::size_t well_index) { return perfpress_[well_index]; }
    const std::vector<double>& perfPress(std::size_t well_index) const { return perfpress_[well_index]; }
    std::vector<double>& perfPress(const std::string& wname) { return perfpress_[wname]; }
    const std::vector<double>& perfPress(const std::string& wname) const { return perfpress_[wname]; }




protected:
    WellContainer<Well::Status> status_;
    WellContainer<std::vector<PerforationData>> well_perf_data_;
    WellContainer<const ParallelWellInfo*> parallel_well_info_;
    WellMapType wellMap_;
    std::vector<double> bhp_;
    std::vector<double> thp_;
    std::vector<double> temperature_;
    WellContainer<std::vector<double>> wellrates_;
    PhaseUsage phase_usage_;

private:
    WellContainer<std::vector<double>> perfrates_;
    WellContainer<std::vector<double>> perfpress_;


    void initSingleWell(const std::vector<double>& cellPressures,
                        const int w,
                        const Well& well,
                        const std::vector<PerforationData>& well_perf_data,
                        const ParallelWellInfo* well_info,
                        const SummaryState& summary_state);
};

} // namespace Opm

#endif // OPM_WELLSTATE_HEADER_INCLUDED
