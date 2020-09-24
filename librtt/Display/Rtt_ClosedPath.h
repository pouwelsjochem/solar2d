//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_ClosedPath_H__
#define _Rtt_ClosedPath_H__

#include "Core/Rtt_Matrix.h"
#include "Core/Rtt_Geometry.h"
#include "Display/Rtt_DisplayTypes.h"
#include "Display/Rtt_ClosedPath.h"
#include "Renderer/Rtt_RenderData.h"

// ----------------------------------------------------------------------------
struct lua_State;

namespace Rtt
{

class Paint;
class Renderer;
class DisplayObject;
class Geometry;
class LuaUserdataProxy;
class MLuaUserdataAdapter;
class Matrix;
class VertexCache;
struct Rect;
struct RenderData;

// ----------------------------------------------------------------------------

// NOTE: ClosedPath instances have no notion of transform
// Only DisplayObjects have that concept!
// 
// Therefore the semantics of Translate in a Path are different than that of an
// Object.  In a path, translate means to translate all cached vertices, or
// if that's not possible, to invalidate any cached vertices.
// In an object, translate means to update its transform and also translate
// any paths it might own.

class ClosedPath
{
	public:
		typedef ClosedPath Self;
		
	public:
		enum {
			kVerticesMask = 0x1,
			kTexVerticesMask = 0x2,
			kIndicesMask = 0x4,
		};

		enum _Constants
		{
			kIsRectPath = 0x1,
			kIsFillWeakReference = 0x2,
			kIsFillLocked = 0x4,
		};
		typedef U8 Properties;

		enum _DirtyMask
		{
			kStroke = 0x1,					// Stroke vertices in renderdata
			kStrokeTexture = 0x2,			// Stroke tex coords in renderdata
			kStrokeSource = 0x4,			// Stroke tesselation
			kStrokeSourceTexture = 0x8,		// Stroke tex tesselation

			kFill = 0x10,					// Fill vertices in renderdata
			kFillTexture = 0x20,			// Fill tex coords in renderdata
			kFillSource = 0x40,				// Fill tesselation
			kFillSourceTexture = 0x80,		// Fill tex tesselation
			
			kFillIndices = 0x100,
			kFillSourceIndices = 0x200,

			kDefault = kFillSource,
		};
		typedef U16 DirtyFlags;

	public:
		ClosedPath( Rtt_Allocator* pAllocator );
		virtual ~ClosedPath();

	public:
		static void UpdateGeometry(
			Geometry& dst, 
			const VertexCache& src, 
			const Matrix& srcToDstSpace, 
			U32 flags, 
			Array<U16> *indices );

	public:
		virtual void Translate( Real dx, Real dy );
		virtual void Update( RenderData& data, const Matrix& srcToDstSpace );
		virtual void UpdateResources( Renderer& renderer ) const = 0;

	public:
		// Returns true if bounds was actually changed; returns false if no-op.
		virtual bool SetSelfBounds( Real width, Real height );
		virtual void GetSelfBounds( Rect& rect ) const = 0;

	public:
		void UpdatePaint( RenderData& data );
		void UpdateColor( RenderData& data, U8 objectAlpha );

	public:
		void SetStrokeData( RenderData *data ) { fStrokeData = data; }

	protected:
		RenderData *GetStrokeData() { return ( HasStroke() ? fStrokeData : NULL ); }

	public:
		Paint* GetFill() { return fFill; }
		const Paint* GetFill() const { return fFill; }
		void SetFill( Paint* newValue );
		void SetFillWeakReference( bool newValue ) { SetProperty( kIsFillWeakReference, newValue ); }
		void SetFillLocked( bool newValue ) { SetProperty( kIsFillLocked, newValue ); }

	public:
		void SwapFill( ClosedPath& rhs );

		Paint* GetStroke() { return fStroke; }
		const Paint* GetStroke() const { return fStroke; }
		void SetStroke( Paint* newValue );

		Rtt_INLINE U8 GetStrokeWidth() const { return fInnerStrokeWidth + fOuterStrokeWidth; }
		Rtt_INLINE U8 GetInnerStrokeWidth() const { return fInnerStrokeWidth; }
		Rtt_INLINE U8 GetOuterStrokeWidth() const { return fOuterStrokeWidth; }

		void SetInnerStrokeWidth( U8 newValue );
		void SetOuterStrokeWidth( U8 newValue );

		bool HasFill() const { return ( NULL != fFill ); }
		bool HasStroke() const { return ( NULL != fStroke ); }
	
		bool IsFillVisible() const;
		bool IsStrokeVisible() const;

	public:
		void Invalidate( DirtyFlags flags ) { fDirtyFlags |= flags; }
		bool IsValid( DirtyFlags flags ) const { return 0 == (fDirtyFlags & flags); }

	protected:
		void SetValid( DirtyFlags flags ) { fDirtyFlags &= (~flags); }

	public:
		DisplayObject *GetObserver() const { return fObserver; }
		void SetObserver( DisplayObject *newValue ) { fObserver = newValue; }

	public:
		const MLuaUserdataAdapter *GetAdapter() const { return fAdapter; }
		void SetAdapter( const MLuaUserdataAdapter *newValue ) { fAdapter = newValue; }
		void PushProxy( lua_State *L ) const;
		void DetachProxy() { fAdapter = NULL; fProxy = NULL; }
	
	public:
		// Use the PropertyMask constants
		// Make properties only read-only to the public
		bool IsProperty( Properties mask ) const { return (fProperties & mask) != 0; }

	protected:
		void ToggleProperty( Properties mask ) { fProperties ^= mask; }
		void SetProperty( Properties mask, bool value )
		{
			const Properties p = fProperties;
			fProperties = ( value ? p | mask : p & ~mask );
		}

	private:
		DisplayObject *fObserver; // weak ptr
		const MLuaUserdataAdapter *fAdapter; // weak ptr
		mutable LuaUserdataProxy *fProxy;

		Paint* fFill; // Only one fill color per path
		Paint* fStroke;

		RenderData *fStrokeData;

		Properties fProperties;
		DirtyFlags fDirtyFlags;
		U8 fInnerStrokeWidth;
		U8 fOuterStrokeWidth;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_ClosedPath_H__
