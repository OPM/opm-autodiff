/*
  Copyright 2019 Equinor ASA

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

#ifndef BLOCKED_MATRIX_HPP
#define BLOCKED_MATRIX_HPP

namespace bda
{

typedef struct {
    double *nnzValues;
    int *colIndices;
    int *rowPointers;
    int Nb;
    int nnzbs;
} BlockedMatrix;

/// Allocate BlockedMatrix and data arrays with given sizes
/// \param[in] Nb               number of blockrows
/// \param[in] nnzbs            number of nonzero blocks
/// \return pointer to BlockedMatrix
template <unsigned int block_size>
BlockedMatrix *allocateBlockedMatrix(int Nb, int nnzbs);

/// Allocate BlockedMatrix, but copy data pointers instead of allocating new memory
/// \param[in] mat              matrix to be copied
/// \return pointer to BlockedMatrix
BlockedMatrix *softCopyBlockedMatrix(BlockedMatrix *mat);

/// Free BlockedMatrix and its data
/// \param[in] mat              matrix to be free'd
void freeBlockedMatrix(BlockedMatrix **mat);

/// Sort a row of matrix elements from a blocked CSR-format
/// \param[inout] colIndices     
/// \param[inout] data           
/// \param[in] left              lower index of data of row
/// \param[in] right             upper index of data of row
template <unsigned int block_size>
void sortBlockedRow(int *colIndices, double *data, int left, int right);

/// Multiply and subtract blocks
/// a = a - (b * c)
/// \param[inout] a              block to be subtracted from
/// \param[in] b                 input block
/// \param[in] c                 input block
template <unsigned int block_size>
void blockMultSub(double *a, double *b, double *c);

/// Perform a 3x3 matrix-matrix multiplication on two blocks
/// \param[in] mat1              input block 1
/// \param[in] mat2              input block 2
/// \param[inout] resMat         output block
template <unsigned int block_size>
void blockMult(double *mat1, double *mat2, double *resMat);

/// Calculate the inverse of a block. This function is specific for only 3x3 block size.
/// \param[in] mat               input block
/// \param[inout] res            output block
void blockInvert3x3(double *mat, double *res);

} // end namespace bda

#endif