//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __Rtt_ShapeAdapterPolygon__
#define __Rtt_ShapeAdapterPolygon__

#include "Display/Rtt_ShapeAdapter.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class TesselatorPolygon;

// ----------------------------------------------------------------------------

class ShapeAdapterPolygon : public ShapeAdapter
{
	public:
		typedef ShapeAdapter Super;

	public:
		static const ShapeAdapterPolygon& Constant();

		static bool InitializeContour(
			lua_State *L, int index, TesselatorPolygon& tesselator );

	protected:
		ShapeAdapterPolygon();

	public:

        virtual StringHash *GetHash( lua_State *L ) const;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // __Rtt_ShapeAdapterPolygon__
