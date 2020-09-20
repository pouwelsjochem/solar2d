//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Display/Rtt_ShapeAdapterPolygon.h"

#include "Core/Rtt_StringHash.h"
#include "Display/Rtt_DisplayTypes.h"
#include "Display/Rtt_ShapePath.h"
#include "Display/Rtt_TesselatorPolygon.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaContext.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

const ShapeAdapterPolygon&
ShapeAdapterPolygon::Constant()
{
	static const ShapeAdapterPolygon sAdapter;
	return sAdapter;
}

bool
ShapeAdapterPolygon::InitializeContour(
	lua_State *L, int index, TesselatorPolygon& tesselator )
{
	bool result = false;

	index = Lua::Normalize( L, index );
	if ( lua_istable( L, index ) )
	{
		ArrayVertex2& contour = tesselator.GetContour();
		Rtt_ASSERT( contour.Length() == 0 );

		// This is used to find the center of the body.
		Rect bounds;

		int numVertices = (int) lua_objlen( L, index ) >> 1;
		for ( int i = 0; i < numVertices; i++ )
		{
			// Lua is one-based, so the first element must be at index 1.
			lua_rawgeti( L, index, ( ( i * 2 ) + 1 ) );

			// Lua is one-based, so the second element must be at index 2.
			lua_rawgeti( L, index, ( ( i * 2 ) + 2 ) );

			Vertex2 v = { luaL_toreal( L, -2 ),
							luaL_toreal( L, -1 ) };
			lua_pop( L, 2 );

			contour.Append( v );
			bounds.Union( v );
		}

		Vertex2 center_offset;
		bounds.GetCenter( center_offset );

		// Offset the contour to center the body around its center of mass.
		for ( int i = 0; i < numVertices; i++ )
		{
			contour[ i ].x -= center_offset.x;
			contour[ i ].y -= center_offset.y;
		}

		tesselator.Invalidate();
		result = true;
	}

	return result;
}

// ----------------------------------------------------------------------------

ShapeAdapterPolygon::ShapeAdapterPolygon()
:	Super( kPolygonType )
{
}

StringHash *
ShapeAdapterPolygon::GetHash( lua_State *L ) const
{
	static const char *keys[] = 
	{
		"",
	};
	static StringHash sHash( *LuaContext::GetAllocator( L ), keys, sizeof( keys ) / sizeof( const char * ), 1, 0, 0, __FILE__, __LINE__ );
	return &sHash;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

