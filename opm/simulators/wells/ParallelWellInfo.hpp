/*
  Copyright 2020 OPM-OP AS

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
#ifndef OPM_PARALLELWELLINFO_HEADER_INCLUDED
#define OPM_PARALLELWELLINFO_HEADER_INCLUDED

#include <dune/common/version.hh>
#include <dune/common/parallel/mpihelper.hh>

#include <memory>

namespace Opm
{

/// \brief Class encapsulating some information about parallel wells
///
/// e.g. It provides a communicator for well information
struct ParallelWellInfo
{
    using MPIComm = typename Dune::MPIHelper::MPICommunicator;
#if DUNE_VERSION_NEWER(DUNE_COMMON, 2, 7)
    using Communication = Dune::Communication<MPIComm>;
#else
    using Communication = Dune::CollectiveCommunication<MPIComm>;
#endif

    /// \brief Deleter that also frees custom MPI communicators
    struct DestroyComm
    {
        void operator()(Communication* comm);
    };

    /// \brief Constructs object using MPI_COMM_SELF
    ParallelWellInfo(const std::string& name = {""});

    /// \brief Constructs object with communication between all rank sharing
    ///        a well
    /// \param well_info Pair of well name and whether local cells might be perforated
    ///        on this rank
    ParallelWellInfo(const std::pair<std::string,bool>& well_info);

    const Communication& communication() const
    {
        return *comm_;
    }

    /// \brief Name of the well.
    std::string name_;
    /// \brief Whether local cells are perforated somewhen
    bool hasLocalCells_ = false;
    /// \brief Whether we own the well and should do reports etc.
    bool isOwner_ = false;
    /// \brief Communication object for the well
    ///
    /// Contains only ranks where this well will perforate local cells.
    std::unique_ptr<Communication, DestroyComm> comm_;
};

bool operator<(const ParallelWellInfo& well1, const ParallelWellInfo& well2);

bool operator==(const ParallelWellInfo& well1, const ParallelWellInfo& well2);

bool operator!=(const ParallelWellInfo& well1, const ParallelWellInfo& well2);

bool operator<(const std::pair<std::string, bool>& pair, const ParallelWellInfo& well);

bool operator<( const ParallelWellInfo& well, const std::pair<std::string, bool>& pair);

bool operator==(const std::pair<std::string, bool>& pair, const ParallelWellInfo& well);

bool operator==(const ParallelWellInfo& well, const std::pair<std::string, bool>& pair);

bool operator!=(const std::pair<std::string, bool>& pair, const ParallelWellInfo& well);

bool operator!=(const ParallelWellInfo& well, const std::pair<std::string, bool>& pair);

} // end namespace Opm
#endif //  OPM_PARALLELWELLINFO_HEADER_INCLUDED