/** \file lvrefcounter.h
	\brief sample ref counter implementation for LVFastRef

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.
	See LICENSE file for details.
*/

#ifndef __LVREFCOUNTER_H_INCLUDED__
#define __LVREFCOUNTER_H_INCLUDED__

/// sample ref counter implementation for LVFastRef
class LVRefCounter
{
	int refCount;
public:
	LVRefCounter() : refCount(0) { }
	void AddRef() { refCount++; }
	int Release() { return --refCount; }
	int getRefCount() { return refCount; }
};

#endif	// __LVREFCOUNTER_H_INCLUDED__
