/** @file lvmatrix.h
	@brief matrix template

	Implements matrix.

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

	This class is unused, saved for history!
*/

#ifndef __LVMATRIX_H_INCLUDED__
#define __LVMATRIX_H_INCLUDED__

#include <stdlib.h>

template<class _Ty > class LVMatrix {
protected:
	int numcols;
	int numrows;
	_Ty ** rows;
public:
	LVMatrix<_Ty> () : numcols(0), numrows(0), rows(NULL) {}
	void Clear() {
		if (rows) {
			if (numrows && numcols) {
				for (int i=0; i<numrows; i++)
					if (rows[i])
						free( rows[i] );
			}
			free( rows );
			rows = NULL;
		}
		numrows = 0;
		numcols = 0;
	}
	~LVMatrix<_Ty> () {
		Clear();
	}

	_Ty * operator [] (int rowindex) {
		if (rows && rowindex >= 0 && rowindex < numrows)
			return rows[rowindex];
		// TODO: throw exception?
		return 0;
	}

	bool SetSize( int nrows, int ncols, _Ty fill_elem ) {
		bool res = false;
		if (nrows <= 0 || ncols <= 0) {
			Clear();
			return true;
		}
		if ( nrows<numrows ) {
			for (int i=nrows; i<numrows; i++)
				free( rows[i] );
			numrows = nrows;
			res = true;
		} else if (nrows>numrows) {
			void* tmp = realloc(rows, sizeof(_Ty*)*nrows);
			if (tmp) {
				bool failed = false;
				rows = (_Ty**)tmp;
				for (int i=numrows; i<nrows; i++) {
					rows[i] = (_Ty*)malloc( sizeof(_Ty) * ncols );
					if (rows[i]) {
						for (int j=0; j<numcols; j++)
							rows[i][j]=fill_elem;
					} else {
						failed = true;
						break;
					}
				}
				if (!failed) {
					numrows = nrows;
					res = true;
				}
			}
		}
		if (ncols>numcols) {
			// TODO: skip useless memory realocation if alloced on previous condition (nrows>numrows)
			bool failed = false;
			for (int i=0; i<numrows; i++) {
				void* tmp = realloc( rows[i], sizeof(_Ty) * ncols );
				if (tmp) {
					rows[i] = (_Ty*)tmp;
					for (int j=numcols; j<ncols; j++)
						rows[i][j]=fill_elem;
				} else {
					failed = true;
					break;
				}
			}
			if (!failed) {
				numcols = ncols;
				res = true;
			}
		}
		return res;
	}
};

#endif	// __LVMATRIX_H_INCLUDED__
