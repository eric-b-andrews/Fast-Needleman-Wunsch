#include "needleman-wunsch.hpp"

// takes in the array to be filled--don't care about the allocation time
void needlemanWunsch(dnaArray s1, dnaArray s2, int* t) {
  // convenience
  long int nRows = s2.size + 1;
  long int nCols = s1.size + 1;
  long int minDim = nRows + (((nCols - nRows) >> LONGBITS) & (nCols - nRows));
  
  // populate first row, column
  // s1 across top, s2 down side
  t[0] = 0;
  

  // OPTIMIZATION: single loop to populate; reduce branching
  // populate first row and column
  for (long int i = 1; i < nCols; ++i) { t[i] = t[i-1] + GAP; }
  for (long int i = 1; i < nRows; ++i) { t[nCols*i] = t[nCols*(i-1)] + GAP; }
  
#pragma omp parallel 
  {
    int a, b, c, m; // temps for max calculation
#pragma omp parallel for schedule(static)
    // (2) do this in strips
    for (long int i = 1; i < nCols; ++i) {
      // loop down diagonal
      // (1) optimize this; do (2)
      long int jMax = i + (((minDim - i) >> LONGBITS) & (minDim - i));
      for (long int j = 1; j < jMax; ++j) {
        // row = j -- move down the rows starting with 0
        // col = i-j -- move left across columns
        m = -(s1.dna[i-j-1] == s2.dna[j-1]);
        a = t[((j-1) * nCols) + i - j - 1] + ((m & MATCH) | (~m & MISMATCH));
        b = t[((j-1) * nCols) + i - j] + GAP;
        c = t[(j * nCols) + i - j - 1] + GAP;
        
        a = a - (((a - b) >> SHIFTBITS) & (a - b));
        a = a - (((a - c) >> SHIFTBITS) & (a - c));
        t[(j * (nCols - 1)) + i] = a;
      }
    }

#pragma omp parallel for schedule(static)
    for (long int i = 1; i < nRows; ++i) {
      long int jMax = (nCols-1)+((((nRows-i)-(nCols-1))>>LONGBITS)&((nRows-i)-(nCols-1)));
      for (long int j = 0; j < jMax; ++j) {
        // row = i+j -- move down rows starting with i
        // col = nCols-j-1 -- move left across columns starting from nCols
        m = -(s1.dna[nCols - j - 2] == s2.dna[i + j - 1]);
        a = t[((i + j) * nCols) - j - 2] + ((m & MATCH) | (~m & MISMATCH));
        b = t[((i + j - 1) * nCols) + (nCols - j - 1)] + GAP;
        c = t[((i + j + 1) * nCols) - j - 2] + GAP;
        
        a = a - (((a - b) >> SHIFTBITS) & (a - b));
        a = a - (((a - c) >> SHIFTBITS) & (a - c));
        t[((i + j + 1) * nCols) - j - 1] = a;
      }
    }
  }
    
  return;
}

#include "driver.cpp"
