/*
  Copyright 2018 SINTEF Digital, Mathematics and Cybernetics.
  Copyright 2018 Statoil Petroleum AS.

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

#ifndef OPM_LINEARSOLVERAMGCL_HEADER_INCLUDED
#define OPM_LINEARSOLVERAMGCL_HEADER_INCLUDED

#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

namespace Opm
{

    class LinearSolverAmgcl
    {
    public:
      LinearSolverAmgcl(boost::property_tree::ptree prm):
	prm_(prm)
      {
	
      }
      void solve(const int sz,
		 const std::vector<int>& ptr,
		 const std::vector<int>& col,
		 const std::vector<double>& val,
		 const std::vector<double>& rhs,
		 std::vector<double>& sol,
		 int& iters,
		 double& error);
    private:
      void solveCPR(const int sz,
		    const std::vector<int>& ptr,
		    const std::vector<int>& col,
		    const std::vector<double>& val,
		    const std::vector<double>& rhs,
		    std::vector<double>& sol,
		    int& iters,
		    double& error);
      void solveRegular(const int sz,
			const std::vector<int>& ptr,
			const std::vector<int>& col,
			const std::vector<double>& val,
			const std::vector<double>& rhs,
			std::vector<double>& sol,
			int& iters,
			double& error);
      void hackScalingFactors(const int sz,
			      const std::vector<int>& ptr,
			      std::vector<double>& val,
			      std::vector<double>& rhs);
      boost::property_tree::ptree prm_;
    };
} // namespace Opm

#endif // OPM_LINEARSOLVERAMGCL_HEADER_INCLUDED
