//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// With contributions from Dianchu Technology
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Display/Rtt_LuaLibGraphics.h"

#include "Corona/CoronaLibrary.h"
#include "Corona/CoronaLua.h"
#include "Rtt_FilePath.h"
#include "Display/Rtt_BitmapMask.h"
#include "Display/Rtt_Display.h"
#include "Display/Rtt_DisplayDefaults.h"
#include "Display/Rtt_ImageFrame.h"
#include "Display/Rtt_ImageSheet.h"
#include "Display/Rtt_ImageSheetPaint.h"
#include "Display/Rtt_ImageSheetUserdata.h"
#include "Display/Rtt_ShaderFactory.h"
#include "Display/Rtt_ShaderTypes.h"
#include "Display/Rtt_TextureResource.h"
#include "Rtt_LuaAux.h"
#include "Rtt_LuaLibSystem.h"
#include "Display/Rtt_BitmapPaint.h"
#include "Rtt_TextureFactory.h"
#include "Rtt_LuaContext.h"
#include "Rtt_LuaLibNative.h"

#include <float.h>

#define ENABLE_DEBUG_PRINT	( 0 )

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

class GraphicsLibrary
{
	public:
		typedef GraphicsLibrary Self;

	public:
		static const char kName[];

	protected:
		GraphicsLibrary( Display& display );
		~GraphicsLibrary();

	public:
		Display& GetDisplay() { return fDisplay; }

	public:
		static int Open( lua_State *L );

	protected:
		static int Initialize( lua_State *L );
		static int Finalizer( lua_State *L );

	public:
		static Self *ToLibrary( lua_State *L );

	public:
		static int newMask( lua_State *L );
		static int newImageSheet( lua_State *L );
		static int defineEffect( lua_State *L );
		static int listEffects( lua_State *L );
		static int newTexture( lua_State *L );
		static int releaseTextures( lua_State *L );
        static int undefineEffect( lua_State *L );

	private:
		Display& fDisplay;
};

// ----------------------------------------------------------------------------

const char GraphicsLibrary::kName[] = "graphics";

// ----------------------------------------------------------------------------

GraphicsLibrary::GraphicsLibrary( Display& display )
:	fDisplay( display )
{
}

GraphicsLibrary::~GraphicsLibrary()
{
}

int
GraphicsLibrary::Open( lua_State *L )
{
	Display *display = (Display *)lua_touserdata( L, lua_upvalueindex( 1 ) );
	Rtt_ASSERT( display );

	// Register __gc callback
	const char kMetatableName[] = __FILE__; // Globally unique string to prevent collision
	CoronaLuaInitializeGCMetatable( L, kMetatableName, Finalizer );

	const luaL_Reg kVTable[] =
	{
		{ "newMask", newMask },
//		{ "newVertexArray", newVertexArray },
		{ "newImageSheet", newImageSheet },
		{ "defineEffect", defineEffect },
		{ "listEffects", listEffects },
		{ "newTexture", newTexture },
		{ "releaseTextures", releaseTextures },
        { "undefineEffect", undefineEffect },

		{ NULL, NULL }
	};

	// Set library as upvalue for each library function
	Self *library = Rtt_NEW( & display.GetRuntime().GetAllocator(), Self( * display ) );

	// Store the library singleton in the registry so it persists
	// using kMetatableName as the unique key.
	CoronaLuaPushUserdata( L, library, kMetatableName ); // push ud
	lua_pushstring( L, kMetatableName ); // push key
	lua_settable( L, LUA_REGISTRYINDEX ); // pops ud, key

	// Leave library" on top of stack
	// Set library as upvalue for each library function
	int result = CoronaLibraryNew( L, kName, "com.coronalabs", 1, 1, kVTable, library );
	/*
	{
		lua_pushlightuserdata( L, library );
		lua_pushcclosure( L, ValueForKey, 1 ); // pop ud
		CoronaLibrarySetExtension( L, -2 ); // pop closure
	}
	*/

	return result;
}

int
GraphicsLibrary::Finalizer( lua_State *L )
{
	Self *library = (Self *)CoronaLuaToUserdata( L, 1 );

	delete library;

	return 0;
}

GraphicsLibrary *
GraphicsLibrary::ToLibrary( lua_State *L )
{
	// library is pushed as part of the closure
	Self *library = (Self *)lua_touserdata( L, lua_upvalueindex( 1 ) );
	return library;
}

// graphics.newMask( filename [, baseDir] )
int
GraphicsLibrary::newMask( lua_State *L )
{
	int result = 0;

	int nextArg = 1;

	// Only required param is "filename"
	// filename [, baseDirectory]
	if ( lua_isstring( L, nextArg ) )
	{
		const char *imageName = lua_tostring( L, nextArg++ );
		
		MPlatform::Directory baseDir = MPlatform::kResourceDir;
		if ( lua_islightuserdata( L, nextArg ) )
		{
			void* p = lua_touserdata( L, nextArg );
			baseDir = (MPlatform::Directory)EnumForUserdata(
				 LuaLibSystem::Directories(),
				 p,
				 MPlatform::kNumDirs,
				 MPlatform::kResourceDir );
			++nextArg;
		}

		GraphicsLibrary *library = GraphicsLibrary::ToLibrary( L );
		result = FilePath::CreateAndPush(
			L, library->GetDisplay().GetAllocator(), imageName, baseDir );
	}

	return result;
}

// graphics.newVertexArray( x1, y1 [,x2, y2, ... ] )
/*
static int
newVertexArray( lua_State *L )
{
	int result = 0;

	Rtt_ASSERT_NOT_IMPLEMENTED();

	return result;
}
*/

// graphics.newImageSheet( filename, [baseDir, ] options )
int
GraphicsLibrary::newImageSheet( lua_State *L )
{
	GraphicsLibrary *library = GraphicsLibrary::ToLibrary( L );
	int result = ImageSheet::CreateAndPush( L, library->GetDisplay().GetAllocator() );
	return result;
}

//static bool
//HasKeyWithValueType( lua_State *L, int index, const char *key, const int valueType )
//{
//	bool result = false;
//
//	if ( lua_istable( L, index ) && key )
//	{
//		lua_getfield( L, index, key );
//		result = ( valueType == lua_type( L, -1 ) );
//		lua_pop( L, 1 );
//	}
//
//	return result;
//}

// graphics.defineEffect( params )
int
GraphicsLibrary::defineEffect( lua_State *L )
{
	GraphicsLibrary *library = GraphicsLibrary::ToLibrary( L );
	Display& display = library->GetDisplay();

	int index = 1; // index of params
	
	ShaderFactory& factory = display.GetShaderFactory();

	lua_pushboolean( L, factory.DefineEffect( L, index ) );
	return 1;
}

// graphics.listEffects( category )
int
GraphicsLibrary::listEffects( lua_State *L )
{
	GraphicsLibrary *library = GraphicsLibrary::ToLibrary( L );

	const ShaderFactory& factory = library->GetDisplay().GetShaderFactory();

	const char *category = lua_tostring( L, 1 );
	ShaderTypes::Category c = ShaderTypes::CategoryForString( category );

	factory.PushList( L, c );
	return 1;
}

//helper funciton to parse lua table to create bitmap resource
SharedPtr<TextureResource> CreateResourceBitmapFromTable(Rtt::TextureFactory &factory, lua_State *L, int index)
{
	SharedPtr<TextureResource> ret;
		
	lua_getfield( L, index, "baseDir" );
	MPlatform::Directory baseDir = LuaLibSystem::ToDirectory( L, -1, MPlatform::kResourceDir );
	lua_pop( L, 1 );
	
	lua_getfield( L, index, "isMask" );
	bool isMask = lua_isboolean( L, -1 ) && lua_toboolean( L, -1 );
	lua_pop( L, 1 );
	
	lua_getfield( L, index, "filename" );
	const char *filename = luaL_checkstring( L, -1);
	if( filename )
	{
		SharedPtr<TextureResource> texSource = factory.FindOrCreate(filename, baseDir, PlatformBitmap::kIsBitsFullResolution, isMask);
		if( texSource.NotNull() )
		{
			factory.Retain(texSource);
			ret = texSource;
		}
	}
	else
	{
		CoronaLuaError( L, "display.newTexture() requires a valid filename" );
	}
	lua_pop( L, 1 );
	
	return ret;
}

//helper funciton to parse lua table to create canvas resource
SharedPtr<TextureResource> CreateResourceCanvasFromTable(Rtt::TextureFactory &factory, lua_State *L, int index, bool isCanvas)
{
	Display &display = factory.GetDisplay();
	
	static unsigned int sNextRenderTextureID = 1;
	SharedPtr<TextureResource> ret;
	
	Real width = -1, height = -1;
	int pixelWidth = -1, pixelHeight = -1;
	
	lua_getfield( L, index, "width" );
	if (lua_isnumber( L, -1 ))
	{
		width = lua_tonumber( L, -1 );
	}
	lua_pop( L, 1 );
	
	lua_getfield( L, index, "height" );
	if (lua_isnumber( L, -1 ))
	{
		height = lua_tonumber( L, -1 );
	}
	lua_pop( L, 1 );
	
	lua_getfield( L, index, "pixelWidth" );
	if (lua_isnumber( L, -1 ))
	{
		pixelWidth = (int)lua_tointeger( L, -1 );
	}
	lua_pop( L, 1 );
	
	lua_getfield( L, index, "pixelHeight" );
	if (lua_isnumber( L, -1 ))
	{
		pixelHeight = (int)lua_tointeger( L, -1 );
	}
	lua_pop( L, 1 );
	
	if( width > 0 && height > 0 )
	{
		
		if (pixelWidth <= 0 || pixelHeight <= 0)
		{
			pixelWidth = Rtt_RealToInt( width );
			pixelHeight = Rtt_RealToInt( height );
			display.ContentToScreen( pixelWidth, pixelHeight );
		}
		
		int texSize = display.GetMaxTextureSize();
		pixelWidth = Min(texSize, pixelWidth);
		pixelHeight = Min(texSize, pixelHeight);

		char filename[30];
		snprintf(filename, 30, "corona://FBOgo_%u", sNextRenderTextureID++);

		SharedPtr<TextureResource> texSource = factory.FindOrCreateCanvas( filename, width, height, pixelWidth, pixelHeight, isCanvas );
		if( texSource.NotNull() )
		{
			factory.Retain(texSource);
			ret = texSource;
		}
	}
	else
	{
		CoronaLuaError( L, "display.newTexture() requires valid width and height" );
	}
	
	return ret;
}

	
// graphics.newTexture(  {type=, filename, [baseDir=], [isMask=], } )
int
GraphicsLibrary::newTexture( lua_State *L )
{
	int result = 0;
	int index = 1;
	SharedPtr<TextureResource> ret;
	
	if( lua_istable( L, index ) )
	{
		lua_getfield( L, index, "type" );
		const char *textureType = lua_tostring( L, -1 );
		if ( textureType )
		{
			if ( 0 == strcmp( "image", textureType ) )
			{
				Self *library = ToLibrary( L );
				Display& display = library->GetDisplay();
				ret = CreateResourceBitmapFromTable(display.GetTextureFactory(), L, index);
			}
			else if ( 0 == strcmp( "canvas", textureType ) || 0 == strcmp( "maskCanvas", textureType ) )
			{
				Self *library = ToLibrary( L );
				Display& display = library->GetDisplay();
				ret = CreateResourceCanvasFromTable(display.GetTextureFactory(), L, index, 0 == strcmp( "maskCanvas", textureType ));
			}
			else
			{
				CoronaLuaError( L, "display.newTexture() unrecognized type" );
			}
		}
		else
		{
			CoronaLuaError( L, "display.newTexture() requires type field in parameters table" );
		}
		lua_pop( L, 1 );
	}
	else
	{
		CoronaLuaError( L, "display.newTexture() requires a table" );
	}
	
	if(	ret.NotNull() )
	{
		ret->PushProxy( L );
		result = 1;
	}
	
	return result;
}

	
	
// graphics.releaseTextures()
int
GraphicsLibrary::releaseTextures( lua_State *L )
{
	int result = 0;
	
	Self *library = ToLibrary( L );
	Display& display = library->GetDisplay();
	
	int index = 1;
	
	TextureResource::TextureResourceType type = TextureResource::kTextureResource_Any;
	
	if( lua_type(L, index) == LUA_TSTRING )
	{
		const char *str = lua_tostring( L, index );
		if( str )
		{
			if ( strcmp(str, "image") == 0 )
			{
				type = TextureResource::kTextureResourceBitmap;
			}
			else if ( strcmp(str, "canvas") == 0 )
			{
				type = TextureResource::kTextureResourceCanvas;
			}
			else if ( strcmp(str, "external") == 0 )
			{
				type = TextureResource::kTextureResourceExternal;
			}
		}
	}
	else if( lua_type(L, index) == LUA_TTABLE )
	{
		lua_getfield( L, index, "type" );
		if( lua_type(L, -1) == LUA_TSTRING )
		{
			const char *str = lua_tostring( L, -1 );
			if( str )
			{
				if ( strcmp(str, "image") == 0 )
				{
					type = TextureResource::kTextureResourceBitmap;
				}
				else if ( strcmp(str, "canvas") == 0 )
				{
					type = TextureResource::kTextureResourceCanvas;
				}
				else if ( strcmp(str, "external") == 0 )
				{
					type = TextureResource::kTextureResourceExternal;
				}
			}
		}
		lua_pop( L, 1 );
	}
	
	
	display.GetTextureFactory().ReleaseByType( type );
	
	return result;
}

// ----------------------------------------------------------------------------

int
GraphicsLibrary::undefineEffect( lua_State *L )
{
    GraphicsLibrary *library = GraphicsLibrary::ToLibrary( L );
    Display& display = library->GetDisplay();

    int index = 1; // index of params
    
    ShaderFactory& factory = display.GetShaderFactory();

    lua_pushboolean( L, factory.UndefineEffect( L, index ) );

    return 1;
}
void
LuaLibGraphics::Initialize( lua_State *L, Display& display )
{
	Rtt_LUA_STACK_GUARD( L );

	FilePath::Initialize( L );
	ImageSheet::Initialize( L );

	lua_pushlightuserdata( L, & display );
	CoronaLuaRegisterModuleLoader( L, GraphicsLibrary::kName, GraphicsLibrary::Open, 1 );

	CoronaLuaPushModule( L, GraphicsLibrary::kName );
	lua_setglobal( L, GraphicsLibrary::kName ); // graphics = library
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------
