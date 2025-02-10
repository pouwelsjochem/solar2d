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
#include "Corona/CoronaGraphics.h"
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
#include "Renderer/Rtt_FormatExtensionList.h"


#include <float.h>

#include <algorithm>
#include <string>
#include <vector>

#define CORONA_SHELL_TRANSFORMS_METATABLE_NAME "graphics.ShellTransforms"

#define ENABLE_DEBUG_PRINT	( 0 )

#define CORONA_SHELL_TRANSFORMS_METATABLE_NAME "graphics.ShellTransforms"

#define ENABLE_DEBUG_PRINT    ( 0 )

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
		static int defineShellTransform( lua_State *L );
		static int defineVertexExtension( lua_State *L );
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
:    fDisplay( display )
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
        { "defineShellTransform", defineShellTransform },
        { "defineVertexExtension", defineVertexExtension },
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
//    bool result = false;
//
//    if ( lua_istable( L, index ) && key )
//    {
//        lua_getfield( L, index, key );
//        result = ( valueType == lua_type( L, -1 ) );
//        lua_pop( L, 1 );
//    }
//
//    return result;
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

// graphics.defineShellTransform( params )
int
GraphicsLibrary::defineShellTransform( lua_State * L )
{
    int ok = 0;

    struct PairWithPriority {
        PairWithPriority()
            : fPriority( 0 )
        {
        }

        std::string fOriginal;
        std::string fModifier;
        int fPriority;
    };

    struct TransformEntry {
        std::string fName;
        std::vector< PairWithPriority > fFindAndInsertAfter;
        std::vector< PairWithPriority > fFindAndReplace;
    };

    struct Transformations {
        std::vector< TransformEntry > mArray;
    };

    struct TransformData {
        const char ** stringList;
        unsigned int count;
        char * newString[1];
    };

    ok = lua_istable( L, 1 );

    if (ok)
    {
        lua_getfield( L, 1, "name" ); // params, name

        ok = lua_isstring( L, -1 );

        if (!ok)
        {
            Rtt_TRACE_SIM( ( "graphics.defineShellTransform(): non-string name" ) );
        }
    }

    else
    {
        Rtt_TRACE_SIM( ( "graphics.defineShellTransform() expected table" ) );
    }

    if (!ok)
    {
        lua_pushboolean( L, 0 ); // params[, name], false

        return 1;
    }

    const char * name = lua_tostring( L, -1 );

    lua_pop( L, 1 ); // params

    Transformations * xforms = (Transformations *)lua_newuserdata( L, sizeof( Transformations ) ); // params, transformations

    new (xforms) Transformations;

    for (lua_pushnil( L ); lua_next( L, 1 ); lua_pop( L, 1 )) // params, transformations[, name, xforms]
    {
        bool isKeyString = LUA_TSTRING == lua_type( L, -2 );

        if (isKeyString && strcmp( lua_tostring( L, -2 ), "name" ) == 0)
        {
            continue;
        }

        else if (isKeyString && lua_istable( L, -1 ))
        {
            xforms->mArray.push_back( TransformEntry() );

            TransformEntry & entry = xforms->mArray.back();

            entry.fName = lua_tostring( L, -2 );

            const char * keys[] = { "findAndInsertAfter", "findAndReplace", NULL };

            for (int i = 0; keys[i]; ++i)
            {
                lua_getfield( L, -1, keys[i] ); // params, transformations, name, xforms, xform?

                if (!lua_isnil( L, -1 ))
                {
                    std::vector< PairWithPriority > * set;
                    
                    if (0 == i)
                    {
                        set = &entry.fFindAndInsertAfter;
                    }
                    
                    else
                    {
                        set = &entry.fFindAndReplace;
                    }

                    for (lua_pushnil( L ); lua_next( L, -2 ); lua_pop( L, 1 )) // params, transformations, name, xforms, xform[, original, modification]
                    {
                        int priority = 0;

                        if (lua_istable( L, -1 ))
                        {
                            lua_getfield( L, -1, "priority" ); // params, transformations, name, xforms, xform, original, modificationTable, priority?

                            if (lua_isnumber( L, -1 ))
                            {
                                priority = lua_tointeger( L, -1 );
                            }

                            else
                            {
                                Rtt_TRACE_SIM( ( "graphics.defineShellTransform(): non-number priority" ) );
                            }

                            lua_pop( L, 1 ); // params, transformations, name, xforms, xform, original, modificationTable
                            lua_getfield( L, -1, "value" ); // params, transformations, name, xforms, xform, original, modificationTable, modification?
                            lua_remove( L, -2 ); // params, transformations, name, xforms, xform, original, modification?
                        }

                        if (LUA_TSTRING == lua_type( L, -2 ) && lua_isstring( L, -1 )) // n.b. harmless to transform second one to string, if number
                        {
                            set->push_back( PairWithPriority() );

                            PairWithPriority & pwp = set->back();

                            pwp.fOriginal = lua_tostring( L, -2 );
                            pwp.fModifier = lua_tostring( L, -1 );
                            pwp.fPriority = priority;
                        }

                        else
                        {
                            if (lua_type( L, -2 ) != LUA_TSTRING)
                            {
                                Rtt_TRACE_SIM( ( "graphics.defineShellTransform(): non-string original value" ) );
                            }

                            if (!lua_isstring( L, -1 ))
                            {
                                Rtt_TRACE_SIM( ( "graphics.defineShellTransform(): non-string modification value" ) );
                            }
                        }
                    }

                    std::sort( set->begin(), set->end(), []( const PairWithPriority & p1, const PairWithPriority & p2 ) {
                        return p1.fPriority < p2.fPriority;
                    });
                }

                lua_pop( L, 1 ); // params, transformations, name, xforms
            }
        }

        else
        {
            if (lua_type( L, -2 ) != LUA_TSTRING)
            {
                Rtt_TRACE_SIM( ( "graphics.defineShellTransform(): non-string source name" ) );
            }

            if (!lua_istable( L, -1 ))
            {
                Rtt_TRACE_SIM( ( "graphics.defineShellTransform(): non-table source value" ) );
            }
        }
    }

    if (luaL_newmetatable( L, CORONA_SHELL_TRANSFORMS_METATABLE_NAME )) // params, transformations, mt
    {
        lua_pushcfunction( L, []( lua_State * L ) {
            (( Transformations * )lua_touserdata( L, 1 ))->~Transformations();

            return 0;
        } ); // params, transformations, mt, GC
        lua_setfield( L, -2, "__gc" ); // params, transformations, mt = { __gc = GC }
    }

    lua_setmetatable( L, -2 ); // params, transformations; transformations.metatable = mt

    CoronaShellTransform transform = {};

    transform.size = sizeof( CoronaShellTransform );

    transform.begin = []( CoronaShellTransformParams * params, void * workSpace, void * )
    {
        TransformData * transformData = static_cast< TransformData * >( workSpace );
        const Transformations * transformations = static_cast< const Transformations * >( params->userData );

        transformData->count = transformations->mArray.size();
        transformData->stringList = static_cast< const char ** >( malloc( params->nsources * sizeof( const char * ) ) );

        for (size_t i = 0; i < transformData->count; ++i)
        {
            transformData->newString[i] = NULL;
        }

        int newStringIndex = 0;

        for (size_t i = 0; i < params->nsources; ++i)
        {
            const char * source = params->sources[i];

            for (const TransformEntry & entry : transformations->mArray)
            {
                if (entry.fName == params->hints[i])
                {
                    std::string updated = source;

                    for (const PairWithPriority & pwp : entry.fFindAndInsertAfter)
                    {
                        size_t pos = updated.find( pwp.fOriginal );

                        if (std::string::npos != pos)
                        {
                            updated.insert( pos + pwp.fOriginal.size(), pwp.fModifier );
                        }
                    }

                    for (const PairWithPriority & pwp : entry.fFindAndReplace)
                    {
						size_t lastPos = std::string::npos;
						U32 repeatCount = 0, overallCount = 0;

                        while (true)
                        {
                            size_t pos = updated.find( pwp.fOriginal );

                            if (std::string::npos == pos)
                            {
                                break;
                            }
							
							// Guard against getting stuck, e.g. if our substitution
							// ens up copying or appending to the input string.
							if (pos == lastPos)
							{
								++repeatCount;
							}
							
							else
							{
								lastPos = pos;
								repeatCount = 0;
							}
							
							++overallCount;
							
							bool tooManyLoops = 10 == repeatCount || 100 == overallCount;

                            updated.replace( pos, pwp.fOriginal.size(), !tooManyLoops ? pwp.fModifier : "\n#error Too many loops\n" );
							
							if (tooManyLoops)
							{
								break;
							}
                        }
                    }

                    source = transformData->newString[newStringIndex++] = strdup( updated.c_str() );
                }
            }

            transformData->stringList[i] = source;
        }

        return transformData->stringList;
    };

    transform.finish = []( void * workSpace, void * )
    {
        TransformData * transformData = static_cast< TransformData * >( workSpace );

        for (size_t i = 0; i < transformData->count; ++i)
        {
            free( transformData->newString[i] );
        }

        free( transformData->stringList );
    };

    transform.workSpace = sizeof( TransformData );

    if (xforms->mArray.size() > 1U)
    {
        transform.workSpace += (xforms->mArray.size() - 1U) * sizeof( char * );
    }

    transform.userData = xforms;

    ok = CoronaShaderRegisterShellTransform( L, name, &transform );

    if (ok)
    {
		GraphicsLibrary *library = GraphicsLibrary::ToLibrary( L );
		ShaderFactory& factory = library->GetDisplay().GetShaderFactory();
		
		factory.AddExternalInfo( L, name, "shellTransform" ); // params
    }

    lua_pushboolean( L, ok ); // params[, transformations], ok

    return 1;
}

// graphics.defineVertexExtension( params )
int
GraphicsLibrary::defineVertexExtension( lua_State *L )
{
    GraphicsLibrary *library = GraphicsLibrary::ToLibrary( L );

    const char * name = NULL;
    int ok = lua_istable( L, 1 );

    if (ok)
    {
        lua_getfield( L, 1, "name" ); // params, name

        ok = lua_isstring( L, -1 );

        if (ok)
        {
            name = lua_tostring( L, -1 );
        }
        
        else
        {
            Rtt_TRACE_SIM( ( "WARNING: `graphics.defineVertexExtension()` has non-string name" ) );
        }
    }

    else
    {
        Rtt_TRACE_SIM( ( "WARNING: `graphics.defineVertexExtension()` expected table" ) );
    }

	VertexAttributeSupport support;
	
	library->GetDisplay().GetVertexAttributes( support );
	
	int instanceByID;
	
	if (ok)
	{
		lua_getfield( L, 1, "instanceByID" ); // params, name, instanceByID

		instanceByID = lua_toboolean( L, -1 ); // params, name
		
		lua_pop( L, 1 ); // 
		
		ok = !instanceByID || NULL != support.suffix;

		if (!ok)
		{
			Rtt_TRACE_SIM( ( "WARNING: `instance-by-ID requested, but not supported" ) );
		}
	}
	
    if (!ok)
    {
        lua_pushboolean( L, 0 ); // params[, name], false

        return 1;
    }
 
    std::vector< CoronaVertexExtensionAttribute > attributes;	
	U32 attribCount = 0;

    ok = false;

    for (int index = 1; ; ++index)
    {
        lua_rawgeti( L, 1, index ); // extension, entry?
        
        if (lua_isnil( L, -1 ))
        {
            ok = true;

            break;
        }
        
        else
        {
            CoronaVertexExtensionAttribute attribute = {};
            
            luaL_checktype( L, -1, LUA_TTABLE );
            lua_getfield( L, -1, "name" ); // extension, entry, name
            
            attribute.name = luaL_checkstring( L, -1 );
            
            lua_pop( L, 1 ); // extension, entry

            lua_getfield( L, -1, "type" ); // extension, entry, type
            
            const char * type = luaL_checkstring( L, -1 );
            const char * typeNames[] = { "byte", "float", "int", NULL };
            
            attribute.type = (CoronaVertexExtensionAttributeType)luaL_checkoption( L, -1, NULL, typeNames );
            
            lua_pop( L, 1 ); // extension, entry
         
            lua_getfield( L, -1, "normalized" ); // extension, entry, normalized
            
            attribute.normalized = lua_toboolean( L, -1 );
            
            lua_pop( L, 1 ); // extension, entry
            
            lua_getfield( L, -1, "componentCount" ); // extension, entry, componentCount
            
            attribute.components = luaL_optinteger( L, -1, 1 );
            
            if (attribute.components < 1 || attribute.components > 4)
            {
                Rtt_TRACE_SIM( ( "WARNING: attribute %s has invalid component count %i", attribute.name, attribute.components ) );
                
                break;
            }
            
            lua_pop( L, 1 ); // extension, entry
                      
            lua_getfield( L, -1, "instancesToReplicate" ); // extension, entry, divisor

            int instancesToReplicate = luaL_optinteger( L, -1, 0 );
            
            lua_pop( L, 1 ); // extension, entry
            
            lua_getfield( L, -1, "windowSize" ); // extension, entry, windowSize
            
            int windowSize = luaL_optinteger( L, -1, 0 );
            
            lua_pop( L, 1 ); // extension, entry
            
            if (instancesToReplicate < 0 || windowSize < 0)
            {
                Rtt_TRACE_SIM( ( "WARNING: `%s` < 0", instancesToReplicate < 0 ? "instancesToReplicate" : "windowSize" ) );
                
                break;
            }

            FormatExtensionList::Group dummyGroup = {};
            
            dummyGroup.divisor = instancesToReplicate;
                        
            if (windowSize)
            {
                attribCount += windowSize;
                
                attribute.windowSize = windowSize;
            }
            
            else
            {
                ++attribCount;
            }
            
            if (attribCount > support.maxCount)
            {
                Rtt_TRACE_SIM( ( "WARNING: iteration %i, attribute count is now %i (maximum %i)", index + 1, attribCount, support.maxCount ) );
                
                break;
            }
            
            if (!dummyGroup.IsInstanceRate())
            {
                lua_getfield( L, -1, "instanced" ); // extension, entry, instanced
                
                if (lua_toboolean( L, -1 ) || windowSize)
                {
                    dummyGroup.divisor = 1;
                }
                
                else if (LUA_TBOOLEAN == lua_type( L, -1 )) // vs. nil
                {
                    Rtt_TRACE_SIM( ( "WARNING: assigned `false` to `instanced` (no-op)" ) );
                }
                
                lua_pop( L, 1 ); // extension, entry
            }
            
            if (dummyGroup.IsInstanceRate())
            {
                if (dummyGroup.NeedsDivisor() && !support.hasDivisors)
                {
                    Rtt_TRACE_SIM( ( "WARNING: %i divisor requested, but not supported", dummyGroup.divisor ) );
                    
                    break;
                }
                
                else if (!support.hasPerInstance)
                {
                    Rtt_TRACE_SIM( ( "WARNING: instance-rate attribute requested, but not supported" ) );
                    
                    break;
                }
            }
            
            attribute.instancesToReplicate = dummyGroup.divisor;
            
            attributes.push_back( attribute );
            
            lua_pop( L, 1 ); // params
        }
    }
    
    lua_pop( L, 1 ); // params

    if (ok && attributes.empty())
    {
        Rtt_TRACE_SIM( ( "WARNING: no attributes found in definition" ) );
        
        ok = false;
    }
    
    if (ok)
    {
        CoronaVertexExtension extension;
        
        extension.size = sizeof( CoronaVertexExtension );
        extension.attributes = attributes.data();
        extension.count = attributes.size();
		extension.instanceByID = instanceByID;
        
        ok = CoronaGeometryRegisterVertexExtension( L, name, &extension );
    }

    lua_pushboolean( L, ok ); // params, ok

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
    
    if ( width <= 0 && pixelWidth > 0 )
    {
        width = pixelWidth;
    }
    if ( height <= 0 && pixelHeight > 0 )
    {
        height = pixelHeight;
    }

    if( width > 0 && height > 0 )
    {
        
        if (pixelWidth <= 0 || pixelHeight <= 0)
        {
            S32 unused = 0, oldPixelWidth = pixelWidth, oldPixelHeight = pixelHeight;
            pixelWidth = Rtt_RealToInt( width );
            pixelHeight = Rtt_RealToInt( height );
            display.ContentToScreen( unused, unused, pixelWidth, pixelHeight );
            
            // if one of the values was specified, use that instead:
            if (oldPixelWidth > 0)
            {
                pixelWidth = oldPixelWidth;
            }
            else if (oldPixelHeight > 0)
            {
                pixelHeight = oldPixelHeight;
            }
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

//helper function to parse lua table to create capture resource
SharedPtr<TextureResource> CreateResourceCaptureFromTable(Rtt::TextureFactory &factory, lua_State *L, int index)
{
	Display &display = factory.GetDisplay();
	
	static unsigned int sNextRenderTextureID = 1;
	SharedPtr<TextureResource> ret;
	
	Real width = -1, height = -1;
	
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
	
	if( width > 0 && height > 0 )
	{
		S32 unused = 0; // n.b. ContentToScreen(x,y) NOT okay for dimensions!
		
		int pixelWidth = Rtt_RealToInt( width );
		int pixelHeight = Rtt_RealToInt( height );
		display.ContentToScreen( unused, unused, pixelWidth, pixelHeight );

		pixelWidth *= display.GetContentToScreenScale();
		pixelHeight *= display.GetContentToScreenScale();
	
		int texSize = display.GetMaxTextureSize();
		pixelWidth = Min(texSize, pixelWidth);
		pixelHeight = Min(texSize, pixelHeight);
		
		char filename[30];
		snprintf(filename, 30, "corona://FBOcap_%u", sNextRenderTextureID++);

		SharedPtr<TextureResource> texSource = factory.FindOrCreateCapture( filename, width, height, pixelWidth, pixelHeight );
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
			else if ( 0 == strcmp( "capture", textureType ) )
			{
				Self *library = ToLibrary( L );
				Display& display = library->GetDisplay();
				ret = CreateResourceCaptureFromTable(display.GetTextureFactory(), L, index);
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
			else if ( strcmp(str, "capture") == 0 )
			{
				type = TextureResource::kTextureResourceCapture;
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
