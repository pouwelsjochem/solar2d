//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"
#include "Core/Rtt_Data.h"

#include "Display/Rtt_LuaLibDisplay.h"

#include "Display/Rtt_BitmapPaint.h"
#include "Display/Rtt_ClosedPath.h"
#include "Display/Rtt_CompositePaint.h"
#include "Display/Rtt_ContainerObject.h"
#include "Display/Rtt_Display.h"
#include "Display/Rtt_DisplayDefaults.h"
#include "Display/Rtt_GradientPaint.h"
#include "Display/Rtt_GroupObject.h"
#include "Display/Rtt_ImageSheetPaint.h"
#include "Display/Rtt_Paint.h"
#include "Display/Rtt_RectObject.h"
#include "Display/Rtt_Scene.h"
#include "Display/Rtt_ShaderFactory.h"
#include "Display/Rtt_ShapeObject.h"
#include "Display/Rtt_ShapePath.h"
#include "Display/Rtt_SnapshotObject.h"
#include "Display/Rtt_StageObject.h"
#include "Display/Rtt_TextureFactory.h"
#include "Renderer/Rtt_Texture.h"

#include "Corona/CoronaLibrary.h"
#include "Corona/CoronaLua.h"

#ifdef Rtt_EXPERIMENTAL_FILTER
#include "Rtt_BufferBitmap.h"
#endif // Rtt_EXPERIMENTAL_FILTER

#include "Display/Rtt_PlatformBitmap.h"
#include "Rtt_ContainerObject.h"
#include "Rtt_ClosedPath.h"
#include "Display/Rtt_ImageFrame.h"
#include "Display/Rtt_ImageSheet.h"
#include "Display/Rtt_ImageSheetUserdata.h"
#include "Rtt_LuaContext.h"
#include "Rtt_LuaLibNative.h"
#include "Rtt_LuaLibSystem.h"
#include "Rtt_LuaProxy.h"
#include "Rtt_Matrix.h"
#include "Rtt_MPlatform.h"
#include "Rtt_Runtime.h"
#include "Display/Rtt_SpriteObject.h"
#include "Renderer/Rtt_Texture.h"

#include "Rtt_Event.h"
#include "Rtt_LuaResource.h"

#include "Core/Rtt_StringHash.h"
#include "Core/Rtt_String.h"

#include <string.h>

#include "Rtt_LuaAux.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

class DisplayLibrary
{
	public:
		typedef DisplayLibrary Self;

	public:
		static const char kName[];

	protected:
		DisplayLibrary( Display& display );
		~DisplayLibrary();

	public:
		Display& GetDisplay() { return fDisplay; }

	public:
		static int Open( lua_State *L );

	protected:
		static int Initialize( lua_State *L );
		static int Finalizer( lua_State *L );

	public:
		static Self *ToLibrary( lua_State *L );

	protected:
		static int ValueForKey( lua_State *L );

	public:
		static ShapeObject* PushImage(
			lua_State *L,
			Vertex2* topLeft,
			BitmapPaint* paint,
			Display& display,
			GroupObject *parent,
			Real w,
			Real h );
		static ShapeObject* PushImage(
			lua_State *L,
			Vertex2* topLeft,
			BitmapPaint* paint,
			Display& display,
			GroupObject *parent );

	public:
		static int newCircle( lua_State *L );
		static int newRect( lua_State *L );
		static int newImage( lua_State *L );
		static int newImageRect( lua_State *L );
		static int newGroup( lua_State *L );
		static int newContainer( lua_State *L );
		static int newSnapshot( lua_State *L );
		static int newSprite( lua_State *L );
		static int getDefault( lua_State *L );
		static int setDefault( lua_State *L );
		static int getCurrentStage( lua_State *L );
		static int collectOrphans( lua_State *L );
		static int capture( lua_State *L );
		static int captureBounds( lua_State *L );
		static int captureScreen( lua_State *L );
        static int save( lua_State *L );
		static int colorSample( lua_State *L );
		static int getSafeAreaInsets( lua_State *L );

	private:
		static void GetRect( lua_State *L, Rect &bounds );

		Display& fDisplay;
};

// ----------------------------------------------------------------------------

const char DisplayLibrary::kName[] = "display";

// ----------------------------------------------------------------------------

DisplayLibrary::DisplayLibrary( Display& display )
:	fDisplay( display )
{
}

DisplayLibrary::~DisplayLibrary()
{
}

int
DisplayLibrary::Open( lua_State *L )
{
	Display *display = (Display *)lua_touserdata( L, lua_upvalueindex( 1 ) );
	Rtt_ASSERT( display );

	// Register __gc callback
	const char kMetatableName[] = __FILE__; // Globally unique string to prevent collision
	CoronaLuaInitializeGCMetatable( L, kMetatableName, Finalizer );

	// Functions in library
	const luaL_Reg kVTable[] =
	{
		{ "newCircle", newCircle },
		{ "newRect", newRect },
		{ "newImage", newImage },
		{ "newImageRect", newImageRect },
		{ "newGroup", newGroup },
		{ "newContainer", newContainer },
		{ "newSnapshot", newSnapshot },
		{ "newSprite", newSprite },
		{ "getDefault", getDefault },
		{ "setDefault", setDefault },
		{ "getCurrentStage", getCurrentStage },
		{ "_collectOrphans", collectOrphans },
		{ "capture", capture },
		{ "captureBounds", captureBounds },
		{ "captureScreen", captureScreen },
		{ "save", save },
		{ "colorSample", colorSample },
		{ "getSafeAreaInsets", getSafeAreaInsets },

		{ NULL, NULL }
	};

	// Set library as upvalue for each library function
	Self *library = Rtt_NEW( & display->GetRuntime().GetAllocator(), Self( * display ) );

	// Store the library singleton in the registry so it persists
	// using kMetatableName as the unique key.
	CoronaLuaPushUserdata( L, library, kMetatableName ); // push ud
	lua_pushstring( L, kMetatableName ); // push key
	lua_settable( L, LUA_REGISTRYINDEX ); // pops ud, key

	// Leave library" on top of stack
	// Set library as upvalue for each library function
	int result = CoronaLibraryNew( L, kName, "com.coronalabs", 1, 1, kVTable, library );
	{
		lua_pushlightuserdata( L, library );
		lua_pushcclosure( L, ValueForKey, 1 ); // pop ud
		CoronaLibrarySetExtension( L, -2 ); // pop closure
	}

	return result;
}

int
DisplayLibrary::Finalizer( lua_State *L )
{
	Self *library = (Self *)CoronaLuaToUserdata( L, 1 );

	delete library;

	return 0;
}

DisplayLibrary *
DisplayLibrary::ToLibrary( lua_State *L )
{
	// library is pushed as part of the closure
	Self *library = (Self *)lua_touserdata( L, lua_upvalueindex( 1 ) );
	return library;
}

int
DisplayLibrary::ValueForKey( lua_State *L )
{
	int result = 1;

	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	const char *key = lua_tostring( L, 2 ); Rtt_ASSERT( key );

	static const char * keys[] = 
	{
		"contentWidth",			// 0
		"contentHeight",		// 1
		"fps",					// 2
		"currentStage",			// 3
		"contentScale",			// 4
		"contentCenterX",		// 5
		"contentCenterY",		// 6
        "deviceWidth",			// 7
        "deviceHeight",			// 8
	};
	
	static StringHash sHash( *LuaContext::GetAllocator( L ), keys, sizeof( keys ) / sizeof(const char *), 9, 26, 7, __FILE__, __LINE__ );
	StringHash *hash = &sHash;

	int index = hash->Lookup( key );
	switch ( index )
	{
	case 0:	//"contentWidth"
		{
			lua_pushinteger( L, display.ContentWidth() );
		}
		break;
	case 1:	// "contentHeight"
		{
			lua_pushinteger( L, display.ContentHeight() );
		}
		break;
	case 2:	// "fps"
		{
			lua_pushinteger( L, display.GetRuntime().GetFPS() );
		}
		break;
	case 3:	// "currentStage"
		{
			display.GetStage()->GetProxy()->PushTable( L );
		}
		break;
	case 4:	// "contentScale"
		{
			lua_pushnumber( L, Rtt_RealToFloat( display.GetScreenToContentScale() ) );
		}
		break;
	case 5:	// "contentCenterX"
		{
			lua_pushnumber( L, 0.5*display.ContentWidth() );
		}
		break;
	case 6:	// "contentCenterY"
		{
			lua_pushnumber( L, 0.5*display.ContentHeight() );
		}
		break;
	case 7:	// "deviceWidth"
		{
			lua_pushnumber( L, display.DeviceWidth() );
		}
		break;
	case 8:	// "deviceHeight"
		{
			lua_pushnumber( L, display.DeviceHeight() );
		}
		break;
	default:
		{
			result = 0;
		}
		break;
	}

	return result;
}

// ----------------------------------------------------------------------------

static GroupObject*
GetParent( lua_State *L, int& nextArg )
{
	GroupObject* parent = NULL;

	if ( lua_istable( L, nextArg ) )
	{
		Rtt_WARN_SIM_PROXY_TYPE( L, nextArg, GroupObject );

		DisplayObject *object = (DisplayObject*)LuaProxy::GetProxyableObject( L, nextArg );
		parent = object ? object->AsGroupObject() : NULL;
		if ( parent )
		{
			++nextArg;
		}
	}

	return parent;
}

static void
AssignDefaultFillColor( const Display& display, ShapeObject& o )
{

	SharedPtr< TextureResource > resource = display.GetTextureFactory().GetDefault();

	Rtt_ASSERT( ! o.GetPath().GetFill() );
	Paint *p = Paint::NewColor(
						display.GetAllocator(),
						resource, display.GetDefaults().GetFillColor() );
	o.SetFill( p );
}

ShapeObject*
DisplayLibrary::PushImage(
	lua_State *L,
	Vertex2* topLeft,
	BitmapPaint* paint,
	Display& display,
	GroupObject *parent,
	Real w,
	Real h )
{
	ShapeObject* v = RectObject::NewRect( display.GetAllocator(), w, h );

	int result = LuaLibDisplay::AssignParentAndPushResult( L, display, v, parent );
	if ( Rtt_VERIFY( result ) )
	{
		if ( topLeft )
		{
			Real x = topLeft->x;
			Real y = topLeft->y;
			v->Translate( x, y );
		}
		v->SetFill( paint );
	}
	else
	{
		Rtt_DELETE( v );
		v = NULL;
	}
	return v;
}

ShapeObject*
DisplayLibrary::PushImage(
	lua_State *L,
	Vertex2* topLeft,
	BitmapPaint* paint,
	Display& display,
	GroupObject *parent )
{
	// Fetch image's width and height. (Might be auto-downscaled here due to texture size limitations.)
	PlatformBitmap *bitmap = paint->GetBitmap();
	Texture *texture = paint->GetTexture(); Rtt_ASSERT( texture );

	// bitmap may be NULL when FBO's are involved
	Real width = Rtt_IntToReal( bitmap ? bitmap->Width() : texture->GetWidth() );
	Real height = Rtt_IntToReal( bitmap ? bitmap->Height() : texture->GetHeight() );

	// Create the image object with the above dimensions.
	ShapeObject* v = PushImage( L, topLeft, paint, display, parent, width, height );

    v->SetObjectDesc("ImageObject");
	
	return v;
}

int
DisplayLibrary::newCircle( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	int nextArg = 1;
	GroupObject *parent = GetParent( L, nextArg );

	Real x = luaL_checkreal( L, nextArg++ );
	Real y = luaL_checkreal( L, nextArg++ );
	Real r = luaL_checkreal( L, nextArg++ );

	ShapePath *path = ShapePath::NewCircle( display.GetAllocator(), r );
	ShapeObject *v = Rtt_NEW( display.GetAllocator(), ShapeObject( path ) );

	int result = LuaLibDisplay::AssignParentAndPushResult( L, display, v, parent );
	AssignDefaultFillColor( display, * v );
	v->Translate( x, y );

	return result;
}
	
int
DisplayLibrary::newRect( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	int nextArg = 1;
	GroupObject *parent = GetParent( L, nextArg );

	Real x = luaL_checkreal( L, nextArg++ );
	Real y = luaL_checkreal( L, nextArg++ );
	Real w = luaL_checkreal( L, nextArg++ );
	Real h = luaL_checkreal( L, nextArg++ );

	ShapeObject* v = RectObject::NewRect( display.GetAllocator(), w, h );
	int result = LuaLibDisplay::AssignParentAndPushResult( L, display, v, parent );

	v->Translate( x, y );
	AssignDefaultFillColor( display, * v );

	return result;
}

// display.newImage( [parentGroup,] filename [, baseDirectory] [, x, y] )
// display.newImage( [parentGroup,] imageSheet, frameIndex [, x, y] )
int
DisplayLibrary::newImage( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	int result = 0;
#ifdef Rtt_DEBUG
	int top = lua_gettop( L );
#endif
	// [parentGroup,]
	int nextArg = 1;
	GroupObject *parent = GetParent( L, nextArg );

	// Only required param is "filename"
	// filename [, baseDirectory]
	if ( lua_isstring( L, nextArg ) )
	{
		const char *imageName = lua_tostring( L, nextArg++ );

		MPlatform::Directory baseDir = MPlatform::kResourceDir;
		if ( lua_islightuserdata( L, nextArg ) )
		{
			void* p = lua_touserdata( L, nextArg );
			baseDir = (MPlatform::Directory)EnumForUserdata( LuaLibSystem::Directories(),
															 p,
															 MPlatform::kNumDirs,
															 MPlatform::kResourceDir );
			++nextArg;
		}

		// [, x, y]
		Vertex2* p = NULL;
		Vertex2 topLeft = { Rtt_REAL_0, Rtt_REAL_0 }; // default to (0,0) if not supplied
		if ( lua_isnumber( L, nextArg ) && lua_isnumber( L, nextArg + 1 ) )
		{
			topLeft.x = luaL_toreal( L, nextArg++ );
			topLeft.y = luaL_toreal( L, nextArg++ );
		}
		p = & topLeft;

		Runtime& runtime = library->GetDisplay().GetRuntime();
		BitmapPaint *paint = BitmapPaint::NewBitmap( runtime, imageName, baseDir, 0x0 );

		if ( paint && paint->GetBitmap() && paint->GetBitmap()->NumBytes() == 0 )
		{
			CoronaLuaWarning(L, "file '%s' does not contain a valid image", imageName);
		}

		if ( paint )
		{
			result = NULL != PushImage( L, p, paint, display, parent );
		}
	}
	else if ( lua_isuserdata( L, nextArg ) )
	{
		ImageSheetUserdata *ud = ImageSheet::ToUserdata( L, nextArg );
		if ( ud )
		{
			nextArg++;
			const AutoPtr< ImageSheet >& sheet = ud->GetSheet();

			int frameIndex = (int) lua_tointeger( L, nextArg );
			if ( frameIndex <= 0 )
			{
				CoronaLuaWarning( L, "display.newImage( imageGroup, frameIndex ) given an invalid frameIndex (%d). Defaulting to 1", frameIndex );
				frameIndex = 1;
			}

			// Map 1-based Lua indices to 0-based C indices
			--frameIndex;
			++nextArg;

			// [, x, y]
			Vertex2* p = NULL;
			Vertex2 topLeft = { Rtt_REAL_0, Rtt_REAL_0 }; // default to (0,0) if not supplied
			if ( lua_isnumber( L, nextArg ) && lua_isnumber( L, nextArg + 1 ) )
			{
				topLeft.x = luaL_toreal( L, nextArg++ );
				topLeft.y = luaL_toreal( L, nextArg++ );
			}
			p = & topLeft;
            
            if( sheet->GetNumFrames() <= frameIndex )
            {
                CoronaLuaWarning( L, "display.newImage( imageGroup, frameIndex ) given an invalid frameIndex (%d). Defaulting to max frame", frameIndex+1 );
                frameIndex = sheet->GetNumFrames()-1;
            }
			const ImageFrame *frame = sheet->GetFrame( frameIndex );
			Real w = Rtt_IntToReal( frame->GetWidth() );
			Real h = Rtt_IntToReal( frame->GetHeight() );

			ImageSheetPaint *paint = ImageSheetPaint::NewBitmap( display.GetAllocator(), sheet, frameIndex );

			if ( paint )
			{
				result = NULL != PushImage( L, p, paint, display, parent, w, h );
			}
		}
	}
	else
	{
		CoronaLuaError( L, "display.newImage() bad argument #%d: filename or image sheet expected, but got %s",
				nextArg, lua_typename( L, lua_type( L, nextArg ) ) );
	}

	Rtt_ASSERT( lua_gettop( L ) == (top + result) );

	return result;
}


// display.newImageRect( [parentGroup,] filename [, baseDirectory], w, h )
// display.newImageRect( imageGroup, frameIndex, w, h )
int
DisplayLibrary::newImageRect( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	int result = 0;

	// [parentGroup,]
	int nextArg = 1;

	// NOTE: GetParent() increments nextArg if parent found
	GroupObject *parent = GetParent( L, nextArg );

	// Only required param is "filename"
	// filename [, baseDirectory]
	if ( LUA_TSTRING == lua_type( L, nextArg ) )
	{
		const char *imageName = lua_tostring( L, nextArg++ );
		
		MPlatform::Directory baseDir = MPlatform::kResourceDir;
		if ( lua_islightuserdata( L, nextArg ) )
		{
			void* p = lua_touserdata( L, nextArg );
			baseDir = (MPlatform::Directory)EnumForUserdata( LuaLibSystem::Directories(),
															 p,
															 MPlatform::kNumDirs,
															 MPlatform::kResourceDir );
			++nextArg;
		}

		// w, h
		// These are required arguments, so only create a new image rect if they are present
		if ( lua_isnumber( L, nextArg ) && lua_isnumber( L, nextArg + 1 ) )
		{
			Real w = luaL_toreal( L, nextArg++ );
			Real h = luaL_toreal( L, nextArg++ );

			Runtime& runtime = library->GetDisplay().GetRuntime();
			BitmapPaint *paint = BitmapPaint::NewBitmap( runtime, imageName, baseDir, PlatformBitmap::kIsBitsFullResolution );

			if ( paint && paint->GetBitmap() && paint->GetBitmap()->NumBytes() == 0 )
			{
				CoronaLuaWarning(L, "file '%s' does not contain a valid image", imageName);
			}
			if ( Rtt_VERIFY( paint ) )
			{
				result = NULL != PushImage( L, NULL, paint, display, parent, w, h );
			}
		}
		else
		{
			bool isWidthValid = lua_isnumber( L, nextArg );
			int narg = ( isWidthValid ? nextArg+1 : nextArg );
			const char *tname = ( isWidthValid ? "height" : "width" );

			CoronaLuaError( L, "display.newImageRect() bad argument #%d: %s expected, but got %s",
					narg, tname, lua_typename( L, lua_type( L, nextArg ) ) );
		}
	}
	else if ( lua_isuserdata( L, nextArg ) )
	{
		ImageSheetUserdata *ud = ImageSheet::ToUserdata( L, nextArg );
		if ( ud )
		{
			nextArg++;
			const AutoPtr< ImageSheet >& sheet = ud->GetSheet();

			int frameIndex = (int) lua_tointeger( L, nextArg );
			if ( frameIndex <= 0 )
			{
				CoronaLuaWarning( L, "display.newImage( imageGroup, frameIndex ) given an invalid frameIndex (%d). Defaulting to 1", frameIndex );
				frameIndex = 1;
			}

			// Map 1-based Lua indices to 0-based C indices
			--frameIndex;
			++nextArg;

			// w, h
			// These are required arguments, so only create a new image rect if they are present
			if ( lua_isnumber( L, nextArg ) && lua_isnumber( L, nextArg + 1 ) )
			{
				Real w = luaL_toreal( L, nextArg++ );
				Real h = luaL_toreal( L, nextArg++ );

				ImageSheetPaint *paint = ImageSheetPaint::NewBitmap( display.GetAllocator(), sheet, frameIndex );

				if ( Rtt_VERIFY( paint ) )
				{
					result = NULL != PushImage( L, NULL, paint, display, parent, w, h );
				}
			}
			else
			{
				bool isWidthValid = lua_isnumber( L, nextArg );
				int narg = ( isWidthValid ? nextArg+1 : nextArg );
				const char *tname = ( isWidthValid ? "height" : "width" );

				CoronaLuaError( L, "display.newImageRect() bad argument #%d: %s expected, but got %s",
						narg, tname, lua_typename( L, lua_type( L, nextArg ) ) );
			}
		}
	}
	else
	{
		CoronaLuaError( L, "display.newImageRect() bad argument #%d: filename or image sheet expected, but got %s",
				nextArg, lua_typename( L, lua_type( L, nextArg ) ) );
	}

	return result;
}

// display.newGroup( [child1 [, child2 [, child3 ... ]]] )
// With no args, create an empty group and set parent to root
// 
// The following is EXPERIMENTAL and undocumented:
// When a child is passed, create a new group whose parent is the child's, 
// set the (x,y) to be the child's, insert the child into the new group, and
// reset the child's transform.  For subsequent child arguments, only
// insert into new group if each of those children has the same parent as the
// first one.
// TODO: What about other transforms (rotations, scale)?  Do these matter?
int
DisplayLibrary::newGroup( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();
	Rtt_Allocator* context = display.GetAllocator();

	GroupObject *o = Rtt_NEW( context, GroupObject( context, NULL ) );
	GroupObject *parent = NULL; // Default parent is root

	DisplayObject *child = NULL;
	if ( ! lua_isnone( L, 1 ) )
	{
		child = (DisplayObject*)LuaProxy::GetProxyableObject( L, 1 );

		// First child determines the parent and origin of group
		parent = child->GetParent();
	}

	// Fetch num arguments *before* pushing result

	int numArgs = lua_gettop( L );
	int result = LuaLibDisplay::AssignParentAndPushResult( L, display, o, parent );

	if ( child )
	{
		// If there are child arguments, then add them to the group "o"
		// Note that these must be done after o's parent is assigned.  
		// This ensures that o's stage is properly set before the children get
		// re-parented; otherwise the children's stage will be NULL.

		Real x = child->GetGeometricProperty( kOriginX );
		Real y = child->GetGeometricProperty( kOriginY );

		for ( int i = 1; i <= numArgs; i++ )
		{
			child = (DisplayObject*)LuaProxy::GetProxyableObject( L, i );

			if ( child && child->GetParent() == parent )
			{
				Rtt_WARN_SIM_PROXY_TYPE( L, i, DisplayObject );
				o->Insert( -1, child, false );
				child->Translate( -x, -y );
			}
			else
			{
				CoronaLuaWarning( L, "display.newGroup() argument #%d not added to group because "
								 "its parent differs from the first argument's original parent", i );
			}
		}

		o->Translate( x, y );
	}

	return result;
}

int
DisplayLibrary::newContainer( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();
	Rtt_Allocator* context = display.GetAllocator();

	// [parentGroup,]
	int nextArg = 1;

	// NOTE: GetParent() increments nextArg if parent found
	GroupObject *parent = GetParent( L, nextArg );

	Real w = luaL_checkreal( L, nextArg++ );
	Real h = luaL_checkreal( L, nextArg++ );

	ContainerObject *o = Rtt_NEW( context, ContainerObject( context, NULL, w, h ) );
	o->Initialize( display );

	int result = LuaLibDisplay::AssignParentAndPushResult( L, display, o, parent );

	return result;
}

// display.newSnapshot( [parent, ] w, h )
int
DisplayLibrary::newSnapshot( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	Rtt_Allocator* context = display.GetAllocator();

	// [parentGroup,]
	int nextArg = 1;

	// NOTE: GetParent() increments nextArg if parent found
	GroupObject *parent = GetParent( L, nextArg );

	Real w = luaL_checkreal( L, nextArg++ );
	Real h = luaL_checkreal( L, nextArg++ );

	SnapshotObject *o = Rtt_NEW( context, SnapshotObject( context, display, w, h ) );

	int result = LuaLibDisplay::AssignParentAndPushResult( L, display, o, parent );
	
	o->Initialize( L, display, w, h );

	return result;
}

// display.newSprite( [parent, ] sheet, width, height, sequenceData )
int
DisplayLibrary::newSprite( lua_State *L )
{
	int result = 0;

	int nextArg = 1;
	GroupObject *parent = GetParent( L, nextArg );

	ImageSheetUserdata *imageSheetUserdata = NULL;
	if ( lua_isuserdata( L, nextArg ) )
	{
		imageSheetUserdata = ImageSheet::ToUserdata( L, nextArg );
	} 
	else
	{
		CoronaLuaError( L, "display.newSprite() bad argument #%d: image sheet expected, but got %s",
				nextArg, lua_typename( L, lua_type( L, nextArg ) ) );
	}

	nextArg++;
	Real width = luaL_checkreal( L, nextArg++ );
	Real height = luaL_checkreal( L, nextArg++ );
	
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();
	Rtt_Allocator *context = display.GetAllocator();

	SpritePlayer& player = display.GetSpritePlayer();
	SpriteObject *spriteObject = SpriteObject::Create( context, player, width, height);

	if ( lua_istable( L, nextArg ) )
	{
		result = LuaLibDisplay::AssignParentAndPushResult( L, display, spriteObject, parent );

		spriteObject->SetFill(ImageSheetPaint::NewBitmap(context, imageSheetUserdata->GetSheet()));
		spriteObject->Initialize();
		
		int numSequences = (int) lua_objlen( L, nextArg );
		for ( int i = 0; i < numSequences; i++ )
		{
			lua_rawgeti( L, nextArg, i+1); // Lua is 1-based
			spriteObject->AddSequence(context, SpriteSequence::Create( context, L, imageSheetUserdata->GetSheet()->GetNumFrames()));
			lua_pop( L, 1 );
		}
	}
	else
	{
		CoronaLuaError( L, "display.newSprite() requires argument #%d to a table containing sequence data", nextArg );
	}

	return result;
}

static int
PushColor( lua_State *L, Color c )
{
	ColorUnion color; color.pixel = c;
	float kInv255 = 1.f / 255.f;
	lua_pushnumber( L, kInv255 * color.rgba.r );
	lua_pushnumber( L, kInv255 * color.rgba.g );
	lua_pushnumber( L, kInv255 * color.rgba.b );
	lua_pushnumber( L, kInv255 * color.rgba.a );

	return 4;
}

int
DisplayLibrary::getDefault( lua_State *L )
{
	int result = 1;

	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	const char *key = lua_tostring( L, 1 );

	DisplayDefaults& defaults = display.GetDefaults();

	if ( Rtt_StringCompare( key, "anchorX" ) == 0 )
	{
		lua_pushnumber( L, defaults.GetAnchorX() );
	}
	else if ( Rtt_StringCompare( key, "anchorY" ) == 0 )
	{
		lua_pushnumber( L, defaults.GetAnchorY() );
	}
	else if ( Rtt_StringCompare( key, "fillColor" ) == 0 )
	{
		Color c = defaults.GetFillColor();
		result = PushColor( L, c );
	}
	else if ( Rtt_StringCompare( key, "background" ) == 0 )
	{
		Color c = defaults.GetClearColor();
		result = PushColor( L, c );
	}
	else if ( Rtt_StringCompare( key, "magTextureFilter" ) == 0 )
	{
		RenderTypes::TextureFilter filter = defaults.GetMagTextureFilter();
		lua_pushstring( L, RenderTypes::StringForTextureFilter( filter ) );
	}
	else if ( Rtt_StringCompare( key, "minTextureFilter" ) == 0 )
	{
		RenderTypes::TextureFilter filter = defaults.GetMinTextureFilter();
		lua_pushstring( L, RenderTypes::StringForTextureFilter( filter ) );
	}
	else if ( Rtt_StringCompare( key, "textureWrapX" ) == 0 )
	{
		RenderTypes::TextureWrap wrap = defaults.GetTextureWrapX();
		lua_pushstring( L, RenderTypes::StringForTextureWrap( wrap ) );
	}
	else if ( Rtt_StringCompare( key, "textureWrapY" ) == 0 )
	{
		RenderTypes::TextureWrap wrap = defaults.GetTextureWrapY();
		lua_pushstring( L, RenderTypes::StringForTextureWrap( wrap ) );
	}
	else if ( Rtt_StringCompare( key, "isShaderCompilerVerbose" ) == 0 )
	{
		bool value = defaults.IsShaderCompilerVerbose();
		lua_pushboolean( L, value ? 1 : 0 );
	}
	else if ( ( Rtt_StringCompare( key, "isAnchorClamped" ) == 0 ) )
	{
		bool value = defaults.IsAnchorClamped();
		lua_pushboolean( L, value ? 1 : 0 );
	}
	else if ( ( Rtt_StringCompare( key, "isImageSheetSampledInsideFrame" ) == 0 ) )
	{
		bool value = defaults.IsImageSheetSampledInsideFrame();
		lua_pushboolean( L, value ? 1 : 0 );
	}
	else if ( key )
	{
		luaL_error( L, "ERROR: display.getDefault() given invalid key (%s)", key );
		result = 0;
	}

	return result;
}

int
DisplayLibrary::setDefault( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	int index = 1;
	const char *key = lua_tostring( L, index++ );

	DisplayDefaults& defaults = display.GetDefaults();
	Color c = LuaLibDisplay::toColor( L, index );

	if ( Rtt_StringCompare( key, "anchorX" ) == 0 )
	{
		float anchorX = lua_tonumber( L, index );
		if ( defaults.IsAnchorClamped() )
		{
			anchorX = Clamp( anchorX, 0.f, 1.f );
		}
		defaults.SetAnchorX( anchorX );
	}
	else if ( Rtt_StringCompare( key, "anchorY" ) == 0 )
	{
		float anchorY = lua_tonumber( L, index );
		if ( defaults.IsAnchorClamped() )
		{
			anchorY = Clamp( anchorY, 0.f, 1.f );
		}
		defaults.SetAnchorY( anchorY );
	}
	else if ( Rtt_StringCompare( key, "fillColor" ) == 0 )
	{
		defaults.SetFillColor( c );
	}
	else if ( Rtt_StringCompare( key, "background" ) == 0 )
	{
		defaults.SetClearColor( c );
		display.Invalidate(); // Invalidate scene so background is updated
	}
	else if ( Rtt_StringCompare( key, "magTextureFilter" ) == 0 )
	{
		const char *value = lua_tostring( L, index );
		RenderTypes::TextureFilter filter = RenderTypes::TextureFilterForString( value );
		defaults.SetMagTextureFilter( filter );
	}
	else if ( Rtt_StringCompare( key, "minTextureFilter" ) == 0 )
	{
		const char *value = lua_tostring( L, index );
		RenderTypes::TextureFilter filter = RenderTypes::TextureFilterForString( value );
		defaults.SetMinTextureFilter( filter );
	}
	else if ( Rtt_StringCompare( key, "textureWrapX" ) == 0 )
	{
		const char *value = lua_tostring( L, index );
		RenderTypes::TextureWrap wrap = RenderTypes::TextureWrapForString( value );
		defaults.SetTextureWrapX( wrap );
	}
	else if ( Rtt_StringCompare( key, "textureWrapY" ) == 0 )
	{
		const char *value = lua_tostring( L, index );
		RenderTypes::TextureWrap wrap = RenderTypes::TextureWrapForString( value );
		defaults.SetTextureWrapY( wrap );
	}
	else if ( Rtt_StringCompare( key, "isShaderCompilerVerbose" ) == 0 )
	{
		bool value = lua_toboolean( L, index ) ? true : false;
		defaults.SetShaderCompilerVerbose( value );
	}
	else if ( ( Rtt_StringCompare( key, "isAnchorClamped" ) == 0 ) )
	{
		bool value = lua_toboolean( L, index ) ? true : false;
		defaults.SetAnchorClamped( value );
	}
	else if ( ( Rtt_StringCompare( key, "isImageSheetSampledInsideFrame" ) == 0 ) )
	{
		bool value = lua_toboolean( L, index ) ? true : false;
		defaults.SetImageSheetSampledInsideFrame( value );
	}
	else if ( key )
	{
		luaL_error( L, "ERROR: display.setDefault() given invalid key (%s)", key );
	}


	return 0;
}

int
DisplayLibrary::getCurrentStage( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	return display.GetStage()->GetProxy()->PushTable( L );
}

int
DisplayLibrary::collectOrphans( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	GroupObject *orphanage = display.Orphanage(); Rtt_ASSERT( orphanage );
	GroupObject::CollectUnreachables( L, display.GetScene(), * orphanage );
	return 0;
}

int
DisplayLibrary::capture( lua_State *L )
{
	if( lua_isnil( L, 1 ) )
	{
		CoronaLuaWarning( L, "display.capture() first parameter was nil. Expected a display object" );
		return 0;
	}

	LuaProxy* proxy = LuaProxy::GetProxy( L, 1 );
	if( ! Rtt_VERIFY( proxy ) )
	{
		return 0;
	}

	DisplayObject* displayObject = (DisplayObject*)(proxy->Object());

	// Default values for options.
	bool cropObjectToScreenBounds = true;

	lua_getfield( L, -1, "captureOffscreenArea" );
	if( lua_isboolean( L, -1 ) )
	{
		cropObjectToScreenBounds = ( ! lua_toboolean( L, -1 ) );
	}
	lua_pop( L, 1 );

	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	// Do a screenshot of the given display object.
	BitmapPaint *paint = display.CaptureDisplayObject( displayObject, cropObjectToScreenBounds );
	if( ! paint )
	{
		CoronaLuaError(L, "display.capture() unable to capture screen. The platform or device might not be supported" );
		return 0;
	}
	
	// Create a bitmap display object and have it returned to Lua.
	// Note: This screenshot will be automatically rendered on top of the display. If the user does
	//       not want to see it, then he/she should do a display.remove() the returned image object.
	Vertex2 topLeft = { Rtt_REAL_0, Rtt_REAL_0 };
	ShapeObject *v = PushImage( L, & topLeft, paint, display, NULL );
	if( ! v )
	{
		return 0;
	}

	// Resize the screenshot to match the size of object that it captured.
	// We have to do this because the screenshot's size is in pixels which
	// will likely be bigger than the display object itself onscreen in
	// content coordinates.
	const Rtt_Real bitmapWidth = Rtt_IntToReal(paint->GetTexture()->GetWidth());
	const Rtt_Real bitmapHeight = Rtt_IntToReal(paint->GetTexture()->GetHeight());

	Rect bounds = displayObject->StageBounds();
	if ( cropObjectToScreenBounds )
	{
		bounds.Intersect(display.GetScreenContentBounds());
	}

	// Scale.
	Rtt_Real xScale = ( (float)( bounds.xMax - bounds.xMin ) / bitmapWidth );
	Rtt_Real yScale = ( (float)( bounds.yMax - bounds.yMin ) / bitmapHeight );
	v->Scale(xScale, yScale, true);

	return 1;
}

int
DisplayLibrary::captureScreen( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	// Do a screenshot.
	BitmapPaint *paint = display.CaptureScreen();
	if( ! paint )
	{
		CoronaLuaError( L, "display.captureScreen() unable to capture screen. The platform or device might not be supported" );
		return 0;
	}

	// Note: This screenshot will be automatically rendered on top of the display. If the user does
	//       not want to see it, then he/she should do a display.remove() the returned image object.
	Vertex2 topLeft = { Rtt_REAL_0, Rtt_REAL_0 };
	ShapeObject *v = PushImage( L, & topLeft, paint, display, NULL );
	if( ! v )
	{
		// Nothing to do.
		return 0;
	}

	// Resize the screenshot display object to fit the screen's bounds in content coordinates.
	// We have to do this because the screenshot's size is in pixels matching the resolution of the device's display,
	// and will likely not match the size of the content region.
	Rtt_Real targetScreenshotScale = display.GetScreenToContentScale(); // TODO: is this correct?
	v->Scale(targetScreenshotScale, targetScreenshotScale, true);

	return 1;
}

void DisplayLibrary::GetRect( lua_State *L, Rect &bounds )
{
	// Self *library = ToLibrary( L );

	// Do not continue if not given any arguments.
	if (lua_gettop(L) <= 0)
	{
		luaL_error(L, "display.captureBounds() expects a bounds table");
	}

	// Verify that the first argument is a Lua table.
	if (!lua_istable(L, 1))
	{
		luaL_error(L, "ERROR: display.captureBounds() given an invalid argument. Was expecting a bounds table but got a %s",
                   lua_typename( L, lua_type( L, 1 ) ) );
	}

	// Fetch the bounds table's xMin value.
	lua_getfield(L, 1, "xMin");
	if (lua_type(L, -1) == LUA_TNUMBER)
	{
		bounds.xMin = lua_tonumber(L, -1);
	}
	else
	{
		luaL_error(L, "ERROR: display.captureBounds() given a bounds table with an invalid or missing 'xMin' entry");
	}
	lua_pop(L, 1);
	
	// Fetch the bounds table's yMin value.
	lua_getfield(L, 1, "yMin");
	if (lua_type(L, -1) == LUA_TNUMBER)
	{
		bounds.yMin = lua_tonumber(L, -1);
	}
	else
	{
		luaL_error(L, "ERROR: display.captureBounds() given a bounds table with an invalid or missing 'yMin' entry");
	}
	lua_pop(L, 1);
	
	// Fetch the bounds table's xMax value.
	lua_getfield(L, 1, "xMax");
	if (lua_type(L, -1) == LUA_TNUMBER)
	{
		bounds.xMax = lua_tonumber(L, -1);
	}
	else
	{
		luaL_error(L, "ERROR: display.captureBounds() given a bounds table with an invalid or missing 'xMax' entry");
	}
	lua_pop(L, 1);
	
	// Fetch the bounds table's yMax value.
	lua_getfield(L, 1, "yMax");
	if (lua_type(L, -1) == LUA_TNUMBER)
	{
		bounds.yMax = lua_tonumber(L, -1);
	}
	else
	{
		luaL_error(L, "ERROR: display.captureBounds() given a bounds table with an invalid or missing 'yMax' entry");
	}
	lua_pop(L, 1);

	// If the min bounds is greater than the max bounds, then swap them.
	if (bounds.xMin > bounds.xMax)
	{
		Swap(bounds.xMin, bounds.xMax);
	}
	if (bounds.yMin > bounds.yMax)
	{
		Swap(bounds.yMin, bounds.yMax);
	}
}

int
DisplayLibrary::captureBounds( lua_State *L )
{
	Rect bounds;
	GetRect( L, bounds );

	// Capture the specified bounds of the screen.
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();
	Runtime *runtime = & display.GetRuntime();

	bounds.Intersect(display.GetScreenContentBounds());

	// Do a screenshot.
	BitmapPaint *paint = display.CaptureBounds( &bounds );
	if( ! paint )
	{
		CoronaLuaError( L, "display.CaptureBounds() unable to capture screen bounds. The platform or device might not be supported" );
		return 0;
	}
	
	// Create a display object for the screenshot and push it into Lua.
	// Note: This screenshot will be automatically rendered on top of the display. If the user does
	//       not want to see it, then he/she should do a "display:remove()" on the returned object.
	Vertex2 topLeft = { Rtt_REAL_0, Rtt_REAL_0 };
	ShapeObject *v = PushImage( L, & topLeft, paint, runtime->GetDisplay(), NULL );
	if( ! v )
	{
		Rtt_DELETE(paint);
		return 0;
	}

	// Resize the screenshot to match the size of the region that it captured in content coordinates.
	// We have to do this because the screenshot's size is in pixels which will likely be larger than
	// the captured region in content coordinates on high resolution displays.
	const Rtt_Real width = Rtt_IntToReal(paint->GetTexture()->GetWidth());
	const Rtt_Real height = Rtt_IntToReal(paint->GetTexture()->GetHeight());
	Rtt_Real xScale = Rtt_RealDiv(Rtt_IntToReal(bounds.xMax - bounds.xMin), width);
	Rtt_Real yScale = Rtt_RealDiv(Rtt_IntToReal(bounds.yMax - bounds.yMin), height);
	v->Scale(xScale, yScale, true);
// TODO: Fix halfW.halfH correction????
	Rtt_Real xOffset = Rtt_RealDiv(Rtt_RealMul(width, xScale) - width, Rtt_REAL_2);
	Rtt_Real yOffset = Rtt_RealDiv(Rtt_RealMul(height, yScale) - height, Rtt_REAL_2);
	v->Translate(xOffset, yOffset);
	
	// Return a display object showing the captured region.
	return 1;
}

int
DisplayLibrary::save( lua_State *L )
{
	if( lua_isnil( L, 1 ) )
	{
		CoronaLuaWarning( L, "display.save() first parameter was nil. Expected a display object" );
		return 0;
	}

	LuaProxy* proxy = LuaProxy::GetProxy( L, 1 );
	if( ! Rtt_VERIFY( proxy ) )
	{
		return 0;
	}

	// Default values for options.
	bool cropObjectToScreenBounds = true;
	ColorUnion backgroundColor;
	bool backgroundColorHasBeenProvided = false;

	lua_getfield( L, -1, "captureOffscreenArea" );
	if( lua_isboolean( L, -1 ) )
	{
		cropObjectToScreenBounds = ( ! lua_toboolean( L, -1 ) );
	}
	lua_pop( L, 1 );

	lua_getfield( L, -1, "backgroundColor" );
	backgroundColorHasBeenProvided = lua_istable( L, -1 );
	if( backgroundColorHasBeenProvided )
	{
		LuaLibDisplay::ArrayToColor( L, -1, backgroundColor.pixel );
	}
	lua_pop( L, 1 );

	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();
	Runtime *runtime = & display.GetRuntime();

	DisplayObject* displayObject = (DisplayObject*)(proxy->Object());

	// Do a screenshot of the given display object.
	BitmapPaint *paint = display.CaptureSave( displayObject,
												cropObjectToScreenBounds,
												( backgroundColorHasBeenProvided ? &backgroundColor : NULL ) );
	if( ! paint )
	{
		CoronaLuaError(L, "display.save() unable to capture screen. The platform or device might not be supported" );

		// Nothing to do.
		return 0;
	}

	const MPlatform& platform = runtime->Platform();

	Rtt::Data<const char> pngBytes(display.GetAllocator());
	platform.SaveBitmap( paint->GetBitmap(), pngBytes );
	lua_pushlstring(L, pngBytes.GetData(), pngBytes.GetLength());

	Rtt_DELETE( paint );

	return 1;
}

int
DisplayLibrary::colorSample( lua_State *L )
{
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();

	float pos_x = lua_tonumber( L, 1 );
	float pos_y = lua_tonumber( L, 2 );

	if( ! Lua::IsListener( L, 3, ColorSampleEvent::kName ) )
	{
		char msg[ 128 ];
		sprintf( msg,
					"ERROR: display.colorSample() requires a function, or an object able to respond to %s",
					ColorSampleEvent::kName );
		luaL_argerror( L, 3, msg );
		return 0;
	}

	// Result callback.
	LuaResource *resource = Rtt_NEW( LuaContext::GetAllocator( L ),
										LuaResource( LuaContext::GetContext( L )->LuaState(),
														3 /*!< Callback index. */ ) );

	RGBA color;
	color.Clear();

	display.ColorSample( pos_x,
							pos_y,
							color );

	ColorSampleEvent e( pos_x,
							pos_y,
							color );

	resource->DispatchEvent( e );

	Rtt_DELETE( resource );

	return 0;
}

int
DisplayLibrary::getSafeAreaInsets( lua_State *L )
{
	Runtime& runtime = * LuaContext::GetRuntime( L );
	Display &display = runtime.GetDisplay();

	Rtt_Real top, left, bottom, right;
	runtime.Platform().GetSafeAreaInsetsPixels(top, left, bottom, right);

	lua_pushnumber( L, top * display.GetScreenToContentScale() );
	lua_pushnumber( L, left * display.GetScreenToContentScale() );
	lua_pushnumber( L, bottom * display.GetScreenToContentScale() );
	lua_pushnumber( L, right * display.GetScreenToContentScale() );
	return 4;
}


// ----------------------------------------------------------------------------

ShapeObject*
LuaLibDisplay::PushImage(
	lua_State *L,
	Vertex2* topLeft,
	BitmapPaint* paint,
	Display& display,
	GroupObject *parent,
	Real w,
	Real h )
{
	return DisplayLibrary::PushImage( L, topLeft, paint, display, parent, w, h );
}

ShapeObject*
LuaLibDisplay::PushImage(
	lua_State *L,
	Vertex2* topLeft,
	BitmapPaint* paint,
	Display& display,
	GroupObject *parent )
{
	return DisplayLibrary::PushImage( L, topLeft, paint, display, parent );
}

void
LuaLibDisplay::Initialize( lua_State *L, Display& display )
{
	Rtt_LUA_STACK_GUARD( L );

	lua_pushlightuserdata( L, & display );
	CoronaLuaRegisterModuleLoader( L, DisplayLibrary::kName, DisplayLibrary::Open, 1 );

	CoronaLuaPushModule( L, DisplayLibrary::kName );
	lua_setglobal( L, DisplayLibrary::kName ); // display = library
}

// NOTE: All transformations should be applied AFTER this call.
// The act of adding an object to a group (i.e. assigning the parent) implicitly
// nukes the child's transform.
int
LuaLibDisplay::AssignParentAndPushResult( lua_State *L, Display& display, DisplayObject* o, GroupObject *pParent )
{
	if ( ! pParent )
	{
		pParent = display.GetStage();
	}
	pParent->Insert( -1, o, false );

	o->AddedToParent( L, pParent );

	o->InitProxy( L );

	// NOTE: V1 compatibility on an object can only be set on creation.
	const DisplayDefaults& defaults = display.GetDefaults();
	o->SetAnchorX( defaults.GetAnchorX() );
	o->SetAnchorY( defaults.GetAnchorY() );

	LuaProxy* proxy = o->GetProxy(); Rtt_ASSERT( proxy );
	return proxy->PushTable( L );
}

Color
LuaLibDisplay::toColorFloat( lua_State *L, int index )
{
	int numArgs = lua_gettop( L ) - index + 1; // add 1 b/c index is 1-based

	ColorUnion c;

	if ( numArgs >= 3 )
	{
		float r = Clamp( lua_tonumber( L, index++ ), 0., 1. );
		float g = Clamp( lua_tonumber( L, index++ ), 0., 1. );
		float b = Clamp( lua_tonumber( L, index++ ), 0., 1. );
		float a = Clamp( (lua_isnone( L, index ) ? 1.0 : lua_tonumber( L, index )), 0., 1. );

		RGBA rgba = { U8(r*255.0f), U8(g*255.0f), U8(b*255.0f), U8(a*255.0f) };
		c.rgba = rgba;
	}
	else
	{
		// Treat first value as a grayscale param
		float value = Clamp( lua_tonumber( L, index++ ), 0., 1. );
		float a = Clamp( (lua_isnone( L, index ) ? 1.0 : lua_tonumber( L, index )), 0., 1. );
		RGBA rgba = { U8(value*255.0f), U8(value*255.0f), U8(value*255.0f), U8(a*255.0f) };
		c.rgba = rgba;
	}

	return c.pixel;
}

Color
LuaLibDisplay::toColorByte( lua_State *L, int index )
{
	int numArgs = lua_gettop( L ) - index + 1; // add 1 b/c index is 1-based

	ColorUnion c;

	if ( numArgs >= 3 )
	{
		int r = (int) lua_tointeger( L, index++ );
		int g = (int) lua_tointeger( L, index++ );
		int b = (int) lua_tointeger( L, index++ );
		int a = (int) ( lua_isnone( L, index ) ? 255 : lua_tointeger( L, index ) );
		RGBA rgba = { (U8)r, (U8)g, (U8)b, (U8)a };
		c.rgba = rgba;
	}
	else
	{
		// Treat first value as a grayscale param
		int value = (int) lua_tointeger( L, index++ );
		int a = (int) ( lua_isnone( L, index ) ? 255 : lua_tointeger( L, index ) );
		RGBA rgba = { (U8)value, (U8)value, (U8)value, (U8)a };
		c.rgba = rgba;
	}

	return c.pixel;
}

int
LuaLibDisplay::PushColorChannels( lua_State *L, Color c )
{
	return PushColor( L, c);
}

Color
LuaLibDisplay::toColor( lua_State *L, int index )
{
	return toColorFloat( L, index );
}

Paint*
LuaLibDisplay::LuaNewColor( lua_State *L, int index )
{
	Color c = Self::toColor( L, index );
	SharedPtr< TextureResource > resource = LuaContext::GetRuntime( L )->GetDisplay().GetTextureFactory().GetDefault();
	Paint* p = Paint::NewColor( LuaContext::GetRuntime( L )->Allocator(), resource, c );
	return p;
}

// { type="image", baseDir=, filename= }
static BitmapPaint *
NewBitmapPaintFromFile( lua_State *L, int paramsIndex )
{
	BitmapPaint *paint = NULL;

	lua_getfield( L, paramsIndex, "filename" );
	const char *imageName = lua_tostring( L, -1 );
	if ( imageName )
	{
		lua_getfield( L, paramsIndex, "baseDir" );
		MPlatform::Directory baseDir =
			LuaLibSystem::ToDirectory( L, -1 );
		lua_pop( L, 1 );

		Runtime *runtime = LuaContext::GetRuntime( L );
		paint = BitmapPaint::NewBitmap( *runtime, imageName, baseDir, 0x0 );
		if ( paint && paint->GetBitmap() && paint->GetBitmap()->NumBytes() == 0 )
		{
			CoronaLuaWarning(L, "file '%s' does not contain a valid image", imageName);
		}
	}
	lua_pop( L, 1 );

	return paint;
}

// { type="image", sheet=, frame= }
static BitmapPaint *
NewBitmapPaintFromSheet( lua_State *L, int paramsIndex )
{
	BitmapPaint *result = NULL;

	lua_getfield( L, paramsIndex, "sheet" );
	if ( lua_isuserdata( L, -1 ) )
	{
		ImageSheetUserdata *ud = ImageSheet::ToUserdata( L, -1 );
		if ( ud )
		{
			const AutoPtr< ImageSheet >& sheet = ud->GetSheet();

			lua_getfield( L, paramsIndex, "frame" );
			int frameIndex = (int) lua_tointeger( L, -1 );
			lua_pop( L, 1 );

			if ( frameIndex <= 0 )
			{
				CoronaLuaWarning( L, "image paint given an invalid frameIndex (%d). Defaulting to 1", frameIndex );
				frameIndex = 1;
			}

			// Map 1-based Lua indices to 0-based C indices
			--frameIndex;

			ImageSheetPaint *paint = ImageSheetPaint::NewBitmap(
				LuaContext::GetAllocator( L ), sheet, frameIndex );

			result = paint;
		}
	}
	lua_pop( L, 1 );

	return result;
}

BitmapPaint *
LuaLibDisplay::LuaNewBitmapPaint( lua_State *L, int paramsIndex )
{
	BitmapPaint *result = NULL;

	if ( ! result )
	{
		result = NewBitmapPaintFromFile( L, paramsIndex );
	}

	if ( ! result )
	{
		result = NewBitmapPaintFromSheet( L, paramsIndex );
	}

	return result;
}

void
LuaLibDisplay::ArrayToColor( lua_State *L, int index, Color& outColor )
{
	int top = lua_gettop( L );

	index = Lua::Normalize( L, index );

	int numArgs = Min( (int)lua_objlen( L, index ), 4 );
	for ( int i = 0; i < numArgs; i++ )
	{
		lua_rawgeti( L, index, (i+1) ); // 1-based
	}

	if ( numArgs > 0 )
	{
		outColor = LuaLibDisplay::toColor( L, top+1 );
	}

	lua_pop( L, numArgs );
}

// { type="gradient", color1={r,g,b,a}, color2={r,g,b,a}, direction= }
GradientPaint *
LuaLibDisplay::LuaNewGradientPaint( lua_State *L, int paramsIndex )
{
	GradientPaint *result = NULL;

	const RGBA kDefault = { 0, 0, 0, 255 };
	ColorUnion color1, color2;
	color1.rgba = kDefault;
	color2.rgba = kDefault;

	lua_getfield( L, paramsIndex, "color1" );
	if ( lua_istable( L, -1 ) )
	{
		ArrayToColor( L, -1, color1.pixel );
	}
	lua_pop( L, 1 );

	lua_getfield( L, paramsIndex, "color2" );
	if ( lua_istable( L, -1 ) )
	{
		ArrayToColor( L, -1, color2.pixel );
	}
	lua_pop( L, 1 );

	GradientPaint::Direction direction = GradientPaint::kDefaultDirection;
	Rtt_Real angle = Rtt_REAL_0;
	lua_getfield( L, paramsIndex, "direction" );
	if ( lua_type( L, -1 ) == LUA_TSTRING )
	{
		direction = GradientPaint::StringToDirection( lua_tostring( L, -1 ) );
	}
	else if ( lua_type( L, -1 ) == LUA_TNUMBER )
	{
		angle = Rtt_FloatToReal(lua_tonumber(L, -1));
	}
	lua_pop( L, 1 );

	Runtime *runtime = LuaContext::GetRuntime( L );
	Display& display = runtime->GetDisplay();
	result = GradientPaint::New( display.GetTextureFactory(), color1.pixel, color2.pixel, direction, angle );

	return result;
}

CompositePaint *
LuaLibDisplay::LuaNewCompositePaint( lua_State *L, int paramsIndex )
{
	CompositePaint *result = NULL;
	Paint *paint0 = NULL;
	Paint *paint1 = NULL;

	lua_getfield( L, paramsIndex, "paint1" );
	if ( lua_istable( L, -1 ) )
	{
		paint0 = LuaNewPaint( L, -1 );
	}
	lua_pop( L, 1 );

	lua_getfield( L, paramsIndex, "paint2" );
	if ( lua_istable( L, -1 ) )
	{
		paint1 = LuaNewPaint( L, -1 );
	}
	lua_pop( L, 1 );

	if ( paint0 && paint1 )
	{
		result = Rtt_NEW( LuaContext::GetAllocator( L ), CompositePaint( paint0, paint1 ) );
	}

	return result;
}

// object.fill = { r, g, b, a }
// object.fill = { type="image", baseDir=, filename= }
// object.fill = { type="image", sheet=, frame= }
// object.fill = { type="gradient", color1={r,g,b,a}, color2={r,g,b,a} }
// object.fill = { type="composite", paint1={}, paint2={} }
// object.fill = { type="camera" }
// TODO: object.fill = other.fill
Paint *
LuaLibDisplay::LuaNewPaint( lua_State *L, int index )
{
	Paint *result = NULL;

	index = Lua::Normalize( L, index );

	if ( lua_istable( L, index ) )
	{
		lua_getfield( L, index, "type" );
		const char *paintType = lua_tostring( L, -1 );
		if ( paintType )
		{
			if ( 0 == strcmp( "image", paintType ) )
			{
				result = LuaNewBitmapPaint( L, index );
			}
			else if ( 0 == strcmp( "gradient", paintType ) )
			{
				result = LuaNewGradientPaint( L, index );
			}
			else if ( 0 == strcmp( "composite", paintType ) )
			{
				result = LuaNewCompositePaint( L, index );
			}
		}
		else
		{
			// Assume it's a color array
			Color c;
			ArrayToColor( L, index, c );
			
			SharedPtr< TextureResource > resource = LuaContext::GetRuntime( L )->GetDisplay().GetTextureFactory().GetDefault();
			result = Paint::NewColor( LuaContext::GetRuntime( L )->Allocator(), resource, c );
		}
		lua_pop( L, 1 );
	}
	else if ( lua_type( L, index ) == LUA_TNUMBER )
	{
		result = LuaLibDisplay::LuaNewColor( L, index );
	}

	return result;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

