/** \file lvrendpagelist.h

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_RENDPAGELIST_H_INCLUDED__
#define __LV_RENDPAGELIST_H_INCLUDED__

#include "lvptrvec.h"
#include "lvrendpageinfo.h"

class LVRendPageList : public LVPtrVector<LVRendPageInfo>
{
public:
	int FindNearestPage( int y, int direction );
	bool serialize( SerialBuf & buf );
	bool deserialize( SerialBuf & buf );
};

#endif	// __LV_RENDPAGELIST_H_INCLUDED__
