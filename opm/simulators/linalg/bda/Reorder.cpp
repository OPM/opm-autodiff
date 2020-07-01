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

#include <vector>
#include <cstring>
#include <algorithm> // for fill()
#include <random>
#include <limits>
#include <sstream>

#include <opm/common/ErrorMacros.hpp>

#include <opm/simulators/linalg/bda/Reorder.hpp>
#include <opm/simulators/linalg/bda/BlockedMatrix.hpp>

namespace bda
{


/* Give every node in the matrix (of which only the sparsity pattern in the
 * form of row pointers and column indices arrays are in the input), a color
 * in the colors array. Also return the amount of colors in the return integer. 
 * This graph-coloring algorithm is based on the Jones-Plassmann algorithm, proposed in:
 * "A Parallel Graph Coloring Heuristic" by M.T. Jones and P.E. Plassmann in SIAM Journal of Scientific Computing 14 (1993) */

template <unsigned int block_size>
int colorBlockedNodes(int rows, const int *rowPointers, const int *colIndices, std::vector<int>& colors, int maxRowsPerColor, int maxColsPerColor)
{
    int left, c;
    const int max_tries = 100;            // since coloring is random, it is possible that a coloring fails. In that case, try again.
    std::vector<int> randoms;
    randoms.resize(rows);

    std::vector<bool> visitedColumns;
    visitedColumns.resize(rows);
    std::fill(visitedColumns.begin(), visitedColumns.end(), false);

    unsigned int colsInColor;
    unsigned int additionalColsInRow;

    for (unsigned int t = 0; t < max_tries; t++) {
        // (re)initialize data for coloring process
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> uniform(0, std::numeric_limits<int>::max());
        {
            for(int i = 0; i < rows; ++i){
                randoms[i] = uniform(gen);
            }
        }
        std::fill(colors.begin(), colors.end(), -1);

        // actually perform coloring
        for (c = 0; c < MAX_COLORS; c++) {
            unsigned int rowsInColor = 0;
            colsInColor = 0;
            for (int i = 0; i < rows; i++)
            {
                bool iMax = true; // true iff you have max random

                // ignore nodes colored earlier
                if ((colors[i] != -1))
                    continue;

                int ir = randoms[i];

                // look at neighbors to check their random number
                for (int k = rowPointers[i]; k < rowPointers[i + 1]; k++) {

                    // ignore nodes colored earlier (and yourself)
                    int j = colIndices[k];
                    int jc = colors[j];
                    if (((jc != -1) && (jc != c)) || (i == j)) {
                        continue;
                    }
                    // The if statement below makes it both true graph coloring and no longer guaranteed to converge
                    if (jc == c) {
                        iMax = false;
                        break;
                    }
                    int jr = randoms[j];
                    if (ir <= jr) {
                        iMax = false;
                    }
                }

                // assign color if you have the maximum random number
                if (iMax) {
                    additionalColsInRow = 0;
                    for (int k = rowPointers[i]; k < rowPointers[i + 1]; k++) {
                        int j = colIndices[k];
                        if (!visitedColumns[j]) {
                            visitedColumns[j] = true;
                            additionalColsInRow += block_size;
                        }
                    }
                    if ((colsInColor + additionalColsInRow) > static_cast<unsigned int>(maxColsPerColor)) {
                        break;
                    }
                    colsInColor += additionalColsInRow;
                    colors[i] = c;
                    rowsInColor += block_size;
                    if ((rowsInColor + block_size - 1) >= static_cast<unsigned int>(maxRowsPerColor)) {
                        break;
                    }
                }

            }
            // Check if graph coloring is done.
            left = 0;
            for (int k = 0; k < rows; k++) {
                if (colors[k] == -1) {
                    left++;
                }
            }
            if (left == 0) {
                return c + 1;
            }
        }
    }

    std::ostringstream oss;
    oss << "Error could not find a graph coloring with " << c << " colors after " << max_tries << " tries.\nNumber of colorless nodes: " << left;
    OPM_THROW(std::logic_error, oss.str());
    return -1;
}


/* Reorder a matrix by a specified input order.
 * Both a to order array, which contains for every node from the old matrix where it will move in the new matrix,
 * and the from order, which contains for every node in the new matrix where it came from in the old matrix.*/

template <unsigned int block_size>
void reorderBlockedMatrixByPattern(BlockedMatrix *mat, int *toOrder, int *fromOrder, BlockedMatrix *rMat) {
    const unsigned int bs = block_size;
    int rIndex = 0;
    int i, k;
    unsigned int j;

    rMat->rowPointers[0] = 0;
    for (i = 0; i < mat->Nb; i++) {
        int thisRow = fromOrder[i];
        // put thisRow from the old matrix into row i of the new matrix
        rMat->rowPointers[i + 1] = rMat->rowPointers[i] + mat->rowPointers[thisRow + 1] - mat->rowPointers[thisRow];
        for (k = mat->rowPointers[thisRow]; k < mat->rowPointers[thisRow + 1]; k++) {
            for (j = 0; j < bs * bs; j++){
                rMat->nnzValues[rIndex * bs * bs + j] = mat->nnzValues[k * bs * bs + j];
            }
            rMat->colIndices[rIndex] = mat->colIndices[k];
            rIndex++;
        }
    }
    // re-assign column indices according to the new positions of the nodes referenced by the column indices
    for (i = 0; i < mat->nnzbs; i++) {
        rMat->colIndices[i] = toOrder[rMat->colIndices[i]];
    }
    // re-sort the column indices of every row.
    for (i = 0; i < mat->Nb; i++) {
        sortBlockedRow<bs>(rMat->colIndices, rMat->nnzValues, rMat->rowPointers[i], rMat->rowPointers[i + 1] - 1);
    }
}

/* Reorder a matrix according to the colors that every node of the matrix has received*/

void colorsToReordering(int Nb, std::vector<int>& colors, int numColors, int *toOrder, int *fromOrder, int *rowsPerColor) {
    int reordered = 0;
    int i, c;
    for (i = 0; i < numColors; i++) {
        rowsPerColor[i] = 0;
    }

    // Find reordering patterns
    for (c = 0; c < numColors; c++) {
        for (i = 0; i < Nb; i++) {
            if (colors[i] == c) {
                rowsPerColor[c]++;
                toOrder[i] = reordered;

                fromOrder[reordered] = i;
                reordered++;
            }
        }
    }
}

// Reorder a vector according to a reordering pattern

template <unsigned int block_size>
void reorderBlockedVectorByPattern(int Nb, double *vector, int *fromOrder, double *rVector) {
    for (int i = 0; i < Nb; i++) {
        for (unsigned int j = 0; j < block_size; j++) {
            rVector[block_size * i + j] = vector[block_size * fromOrder[i] + j];
        }
    }
}


/* Check is operations on a node in the matrix can be started
 * A node can only be started if all nodes that it depends on during sequential execution have already completed.*/

bool canBeStarted(const int rowIndex, const int *rowPointers, const int *colIndices, const std::vector<bool>& doneRows) {
    bool canStart = !doneRows[rowIndex];
    int i, thisDependency;
    if (canStart) {
        for (i = rowPointers[rowIndex]; i < rowPointers[rowIndex + 1]; i++) {
            thisDependency = colIndices[i];
            // Only dependencies on rows that should execute before the current one are relevant
            if (thisDependency >= rowIndex)
                break;
            // Check if dependency has been resolved
            if (!doneRows[thisDependency]) {
                return false;
            }
        }
    }
    return canStart;
}

/*
 * The level scheduling of a non-symmetric, blocked matrix requires access to a CSC encoding and a CSR encoding of the sparsity pattern of the input matrix.
 * This function is based on a standard level scheduling algorithm, like the one described in:
 * "Iterative methods for Sparse Linear Systems" by Yousef Saad in section 11.6.3 
 */

int *findLevelScheduling(int *CSRColIndices, int *CSRRowPointers, int *CSCRowIndices, int *CSCColPointers, int Nb, int *numColors, int *toOrder, int* fromOrder) {
    int activeRowIndex = 0, colorEnd, nextActiveRowIndex = 0;
    int thisRow;
    std::vector<bool> doneRows(Nb, false);
    std::vector<int> rowsPerColor;
    rowsPerColor.reserve(Nb);
    int *resRowsPerColor;

    std::vector <int> rowsToStart;

    // find starting rows: rows that are independent from all rows that come before them.
    for (thisRow = 0; thisRow < Nb; thisRow++) {
        if (canBeStarted(thisRow, CSCColPointers, CSCRowIndices, doneRows)) {
            fromOrder[nextActiveRowIndex] = thisRow;
            toOrder[thisRow] = nextActiveRowIndex;
            nextActiveRowIndex++;
        }
    }
    // 'do' compute on all active rows
    for (colorEnd = 0; colorEnd < nextActiveRowIndex; colorEnd++) {
        doneRows[fromOrder[colorEnd]] = true;
    }

    rowsPerColor.emplace_back(nextActiveRowIndex - activeRowIndex);

    while (colorEnd < Nb) {
        // Go over all rows active from the last color, and check which of their neighbours can be activated this color
        for (; activeRowIndex < colorEnd; activeRowIndex++) {
            thisRow = fromOrder[activeRowIndex];

            for (int i = CSCColPointers[thisRow]; i < CSCColPointers[thisRow + 1]; i++) {
                int thatRow = CSCRowIndices[i];

                if (canBeStarted(thatRow, CSRRowPointers, CSRColIndices, doneRows)) {
                    rowsToStart.emplace_back(thatRow);
                }
            }
        }
        // 'do' compute on all active rows
        for (unsigned int i = 0; i < rowsToStart.size(); i++) {
            thisRow = rowsToStart[i];
            if (!doneRows[thisRow]) {
                doneRows[thisRow] = true;
                fromOrder[nextActiveRowIndex] = thisRow;
                toOrder[thisRow] = nextActiveRowIndex;
                nextActiveRowIndex++;
            }
        }
        colorEnd = nextActiveRowIndex;
        rowsPerColor.emplace_back(nextActiveRowIndex - activeRowIndex);
    }
    // Crop the rowsPerColor array to it minimum size.
    resRowsPerColor = new int[rowsPerColor.size()];
    for (unsigned int i = 0; i < rowsPerColor.size(); i++) {
        resRowsPerColor[i] = rowsPerColor[i];
    }

    *numColors = rowsPerColor.size();

    return resRowsPerColor;
}

/* Perform the complete graph coloring algorithm on a matrix. Return an array with the amount of nodes per color.*/

template <unsigned int block_size>
int* findGraphColoring(const int *colIndices, const int *rowPointers, int Nb, int maxRowsPerColor, int maxColsPerColor, int *numColors, int *toOrder, int* fromOrder) {
    std::vector<int> rowColor;
    rowColor.resize(Nb);
    int *rowsPerColor = new int[MAX_COLORS];

    *numColors = colorBlockedNodes<block_size>(Nb, rowPointers, colIndices, rowColor, maxRowsPerColor, maxColsPerColor);

    colorsToReordering(Nb, rowColor, *numColors, toOrder, fromOrder, rowsPerColor);

    // The rowsPerColor array contains a non-zero value for each color denoting how many rows are in that color. It has a size of *numColors.

    return rowsPerColor;
}

// based on the scipy package from python, scipy/sparse/sparsetools/csr.h on github
void csrPatternToCsc(int *CSRColIndices, int *CSRRowPointers, int *CSCRowIndices, int *CSCColPointers, int Nb) {

    int nnz = CSRRowPointers[Nb];

    // compute number of nnzs per column
    std::fill(CSCColPointers, CSCColPointers + Nb, 0);

    for (int n = 0; n < nnz; ++n) {
        CSCColPointers[CSRColIndices[n]]++;
    }

    // cumsum the nnz per col to get CSCColPointers
    for (int col = 0, cumsum = 0; col < Nb; ++col) {
        int temp = CSCColPointers[col];
        CSCColPointers[col] = cumsum;
        cumsum += temp;
    }
    CSCColPointers[Nb] = nnz;

    for (int row = 0; row < Nb; ++row) {
        for (int j = CSRRowPointers[row]; j < CSRRowPointers[row + 1]; ++j) {
            int col = CSRColIndices[j];
            int dest = CSCColPointers[col];
            CSCRowIndices[dest] = row;
            CSCColPointers[col]++;
        }
    }

    for (int col = 0, last = 0; col <= Nb; ++col) {
        int temp = CSCColPointers[col];
        CSCColPointers[col] = last;
        last = temp;
    }
}


#define INSTANTIATE_BDA_FUNCTIONS(n)                                                               \
template int colorBlockedNodes<n>(int, const int *, const int *, std::vector<int>&, int, int);     \
template void reorderBlockedMatrixByPattern<n>(BlockedMatrix *, int *, int *, BlockedMatrix *);    \
template void reorderBlockedVectorByPattern<n>(int, double*, int*, double*);                       \
template int* findGraphColoring<n>(const int *, const int *, int, int, int, int *, int *, int *);  \

INSTANTIATE_BDA_FUNCTIONS(1);
INSTANTIATE_BDA_FUNCTIONS(2);
INSTANTIATE_BDA_FUNCTIONS(3);
INSTANTIATE_BDA_FUNCTIONS(4);

#undef INSTANTIATE_BDA_FUNCTIONS

} //namespace bda