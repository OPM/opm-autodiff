/*
  Copyright 2019 SINTEF Digital, Mathematics and Cybernetics.

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

#ifndef OPM_GET_QUASI_IMPES_WEIGHTS_HEADER_INCLUDED
#define OPM_GET_QUASI_IMPES_WEIGHTS_HEADER_INCLUDED

#include <algorithm>
#include <cmath>

namespace Opm
{

namespace Details
{
    template <class DenseMatrix>
    DenseMatrix transposeDenseMatrix(const DenseMatrix& M)
    {
        DenseMatrix tmp;
        for (int i = 0; i < M.rows; ++i)
            for (int j = 0; j < M.cols; ++j)
                tmp[j][i] = M[i][j];

        return tmp;
    }
} // namespace Details

namespace Amg
{
            Vector getStorageWeights() const
        {
            Vector weights(rhs_->size());
            BlockVector rhs(0.0);
            rhs[pressureVarIndex] = 1.0;
            int index = 0;
            ElementContext elemCtx(simulator_);
            const auto& vanguard = simulator_.vanguard();
            auto elemIt = vanguard.gridView().template begin</*codim=*/0>();
            const auto& elemEndIt = vanguard.gridView().template end</*codim=*/0>();
            for (; elemIt != elemEndIt; ++elemIt) {
                const Element& elem = *elemIt;
                elemCtx.updatePrimaryStencil(elem);
                elemCtx.updatePrimaryIntensiveQuantities(/*timeIdx=*/0);
                Dune::FieldVector<Evaluation, numEq> storage;
                unsigned threadId = ThreadManager::threadId();
                simulator_.model().localLinearizer(threadId).localResidual().computeStorage(storage,elemCtx,/*spaceIdx=*/0, /*timeIdx=*/0);
                Scalar extrusionFactor = elemCtx.intensiveQuantities(0, /*timeIdx=*/0).extrusionFactor();
                Scalar scvVolume = elemCtx.stencil(/*timeIdx=*/0).subControlVolume(0).volume() * extrusionFactor;
                Scalar storage_scale = scvVolume / elemCtx.simulator().timeStepSize();
                MatrixBlockType block;
                double pressure_scale = 50e5;
                for (int ii = 0; ii < numEq; ++ii) {
                    for (int jj = 0; jj < numEq; ++jj) {
                        block[ii][jj] = storage[ii].derivative(jj)/storage_scale;
                        if (jj == pressureVarIndex) {
                            block[ii][jj] *= pressure_scale;
                        }
                    }
                }
                BlockVector bweights;
                MatrixBlockType block_transpose = Opm::transposeDenseMatrix(block);
                block_transpose.solve(bweights, rhs);
                bweights /= 1000.0; // given normal densities this scales weights to about 1.
                weights[index] = bweights;
                ++index;
            }
            return weights;
        }

    template <class Matrix, class Vector>
    
    void getQuasiImpesWeights(const Matrix& matrix, const int pressureVarIndex, const bool transpose, Vector& weights)
    {
        using VectorBlockType = typename Vector::block_type;
        using MatrixBlockType = typename Matrix::block_type;
        const Matrix& A = matrix;
        VectorBlockType rhs(0.0);
        rhs[pressureVarIndex] = 1.0;
        const auto endi = A.end();
        for (auto i = A.begin(); i != endi; ++i) {
            const auto endj = (*i).end();
            MatrixBlockType diag_block(0.0);
            for (auto j = (*i).begin(); j != endj; ++j) {
                if (i.index() == j.index()) {
                    diag_block = (*j);
                    break;
                }
            }
            VectorBlockType bweights;
            if (transpose) {
                diag_block.solve(bweights, rhs);
            } else {
                auto diag_block_transpose = Opm::Details::transposeDenseMatrix(diag_block);
                diag_block_transpose.solve(bweights, rhs);
            }
            double abs_max = *std::max_element(
                bweights.begin(), bweights.end(), [](double a, double b) { return std::fabs(a) < std::fabs(b); });
            bweights /= std::fabs(abs_max);
            weights[i.index()] = bweights;
        }
        // return weights;
    }

    template <class Matrix, class Vector>
    Vector getQuasiImpesWeights(const Matrix& matrix, const int pressureVarIndex, const bool transpose)
    {
        Vector weights(matrix.N());
        getQuasiImpesWeights(matrix, pressureVarIndex, transpose, weights);
        return weights;
    }

} // namespace Amg

} // namespace Opm

#endif // OPM_GET_QUASI_IMPES_WEIGHTS_HEADER_INCLUDED
