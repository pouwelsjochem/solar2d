//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_LuaProxyVTable_H__
#define _Rtt_LuaProxyVTable_H__

#include "Rtt_LuaProxy.h"
#include "Core/Rtt_String.h"

// ----------------------------------------------------------------------------

struct lua_State;

namespace Rtt
{

class DisplayObject;
class GroupObject;
class LuaGroupObjectProxyVTable;

// ----------------------------------------------------------------------------

#define Rtt_ASSERT_PROXY_TYPE( L, index, T )					\
	Rtt_ASSERT(													\
		LuaProxyVTable::IsProxyUsingCompatibleDelegate(			\
			LuaProxy::GetProxy((L),(index)),					\
			Lua ## T ## ProxyVTable::Constant() ) )


#define Rtt_WARN_SIM_PROXY_TYPE2( L, index, T, API_T )					\
	Rtt_WARN_SIM(														\
		Rtt_VERIFY( LuaProxyVTable::IsProxyUsingCompatibleDelegate(		\
			LuaProxy::GetProxy((L),(index)),							\
			Lua ## T ## ProxyVTable::Constant() ) ),					\
		( "ERROR: Argument(%d) is not a %s\n", index, #API_T ) )

#define Rtt_WARN_SIM_PROXY_TYPE( L, index, T )	Rtt_WARN_SIM_PROXY_TYPE2( L, index, T, T )

// ----------------------------------------------------------------------------

class LuaProxyVTable
{
	public:
		typedef LuaProxyVTable Self;

	public:
		#if defined( Rtt_DEBUG ) || defined( Rtt_AUTHORING_SIMULATOR )
		// Proxy's delegate or an ancestor must match expected
		static bool IsProxyUsingCompatibleDelegate( const LuaProxy* proxy, const Self& expected );
		#endif // Rtt_DEBUG

	public:
		static Self* GetSelf( lua_State *L, int index );
		static int __index( lua_State *L );
		static int __newindex( lua_State *L );
		static int __gcMeta( lua_State *L );

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const = 0;
		virtual bool SetValueForKey( lua_State *L, MLuaProxyable& object, const char key[], int valueIndex ) const;
//		virtual int Length( lua_State *L ) const;
		virtual const LuaProxyVTable& Parent() const;

    bool DumpObjectProperties( lua_State *L, const MLuaProxyable& object, const char **keys, const int numKeys, String& result ) const;
};

// ----------------------------------------------------------------------------

class LuaDisplayObjectProxyVTable : public LuaProxyVTable
{
	public:
		typedef LuaDisplayObjectProxyVTable Self;
		typedef LuaProxyVTable Super;

	public:
		static const Self& Constant();

	public:
		static int translate( lua_State *L );
		static int scale( lua_State *L );
		static int rotate( lua_State *L );

	protected:
		LuaDisplayObjectProxyVTable() {}

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const;
		virtual bool SetValueForKey( lua_State *L, MLuaProxyable& object, const char key[], int valueIndex ) const;
		
		// Removes child at index from parent and pushes onto the stack. Pushes nil
		// if index is invalid. If isOrphan is false, converts object into plain Lua table,
		// freeing underlying DisplayObject. If true, no conversion takes place.
		static void PushAndRemove( lua_State *L, GroupObject* parent, S32 index );
};

class LuaLineObjectProxyVTable : public LuaDisplayObjectProxyVTable
{
	public:
		typedef LuaLineObjectProxyVTable Self;
		typedef LuaDisplayObjectProxyVTable Super;

	public:
		static const Self& Constant();

	public:
		static int setStrokeColor( lua_State *L );
		static int setStroke( lua_State *L );
		static int setStroke( lua_State *L, int valueIndex );
		static int append( lua_State *L );
		static int setAnchorSegments( lua_State *L, int valueIndex );

	protected:
		LuaLineObjectProxyVTable() {}

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const;
		virtual bool SetValueForKey( lua_State *L, MLuaProxyable& object, const char key[], int valueIndex ) const;
		virtual const LuaProxyVTable& Parent() const;
};

class LuaShapeObjectProxyVTable : public LuaDisplayObjectProxyVTable
{
	public:
		typedef LuaShapeObjectProxyVTable Self;
		typedef LuaDisplayObjectProxyVTable Super;

	public:
		static const Self& Constant();

	public:
		static int setFillColor( lua_State *L );
		static int setStrokeColor( lua_State *L );
		static int setFill( lua_State *L, int valueIndex );
		static int setStroke( lua_State *L, int valueIndex );

	protected:
		LuaShapeObjectProxyVTable() {}

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const;
		virtual bool SetValueForKey( lua_State *L, MLuaProxyable& object, const char key[], int valueIndex ) const;
		virtual const LuaProxyVTable& Parent() const;
};

class LuaSnapshotObjectProxyVTable : public LuaShapeObjectProxyVTable
{
	public:
		typedef LuaSnapshotObjectProxyVTable Self;
		typedef LuaShapeObjectProxyVTable Super;

	public:
		static const Self& Constant();

	public:
		static int Append( lua_State *L );
		static int Invalidate( lua_State *L );

	protected:
		LuaSnapshotObjectProxyVTable() {}

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const;
		virtual bool SetValueForKey( lua_State *L, MLuaProxyable& object, const char key[], int valueIndex ) const;

		virtual const LuaProxyVTable& Parent() const;
};

class LuaCompositeObjectProxyVTable : public LuaDisplayObjectProxyVTable
{
	public:
		typedef LuaCompositeObjectProxyVTable Self;
		typedef LuaDisplayObjectProxyVTable Super;

	public:
		static const Self& Constant();

	protected:
		LuaCompositeObjectProxyVTable() {}

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const;
		virtual const LuaProxyVTable& Parent() const;
};

class LuaGroupObjectProxyVTable : public LuaDisplayObjectProxyVTable
{
	public:
		typedef LuaGroupObjectProxyVTable Self;
		typedef LuaDisplayObjectProxyVTable Super;

	public:
		static const Self& Constant();

	public:
		static int Insert( lua_State *L, GroupObject *parent );
		static int insert( lua_State *L );
		static int Remove( lua_State *L, GroupObject *parent );
		static int Remove( lua_State *L );
		static int PushChild( lua_State *L, const GroupObject& o );

	protected:
		int PushMethod( lua_State *L, const GroupObject& o, const char *key ) const;

	protected:
		LuaGroupObjectProxyVTable() {}

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const;
		virtual bool SetValueForKey( lua_State *L, MLuaProxyable& object, const char key[], int valueIndex ) const;
		virtual const LuaProxyVTable& Parent() const;
};

class LuaStageObjectProxyVTable : public LuaGroupObjectProxyVTable
{
	public:
		typedef LuaStageObjectProxyVTable Self;
		typedef LuaGroupObjectProxyVTable Super;

	public:
		static const Self& Constant();

	public:
		static int setFocus( lua_State *L );

	protected:
		LuaStageObjectProxyVTable() {}

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const;
		virtual const LuaProxyVTable& Parent() const;
};

// ----------------------------------------------------------------------------

class LuaSpriteObjectProxyVTable : public LuaShapeObjectProxyVTable
{
	public:
		typedef LuaSpriteObjectProxyVTable Self;
		typedef LuaShapeObjectProxyVTable Super;

	public:
		static const Self& Constant();

	public:
		static int play( lua_State *L );
		static int pause( lua_State *L );
		static int setSequence( lua_State *L );
		static int setFrame( lua_State *L );

	public:
		LuaSpriteObjectProxyVTable() {}

	public:
		virtual int ValueForKey( lua_State *L, const MLuaProxyable& object, const char key[] ) const;
		virtual bool SetValueForKey( lua_State *L, MLuaProxyable& object, const char key[], int valueIndex ) const;
		virtual const LuaProxyVTable& Parent() const;
};
	
// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_Matrix_H__
