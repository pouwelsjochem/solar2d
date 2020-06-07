//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_GPUStream_H__
#define _Rtt_GPUStream_H__

// ----------------------------------------------------------------------------

#include "Rtt_RenderingStream.h"

#include "Display/Rtt_Paint.h"
#include "Rtt_GPU.h"

#if defined( Rtt_AUTHORING_SIMULATOR ) || defined( Rtt_ANDROID_ENV )
#define RTT_SURFACE_ROTATION
#endif

// ----------------------------------------------------------------------------

namespace Rtt
{

class SurfaceProperties;
class VertexCache;

// ----------------------------------------------------------------------------

class GPUStream : public RenderingStream
{
	public:
		typedef RenderingStream Super;
		typedef GPUStream Self;

	public:
		enum
		{
			kMaxTextureStackDepth = 32
		};

	public:
		static int GetMaxTextureUnits();

	public:
		static GLenum GetDataType();

	public:
		GPUStream( Rtt_Allocator* );
		virtual ~GPUStream();

		virtual void Initialize(
						const PlatformSurface& surface,
						DeviceOrientation::Type contentOrientation );

		virtual void PrepareToRender();

		virtual S32 GetRenderedContentWidth() const;
		virtual S32 GetRenderedContentHeight() const;
		virtual Rtt_Real ViewableContentWidth() const;
		virtual Rtt_Real ViewableContentHeight() const;
		virtual Rtt_Real ActualContentWidth() const;
		virtual Rtt_Real ActualContentHeight() const;
		virtual void ContentToPixels( S32& x, S32& y, S32& w, S32& h ) const;

	protected:
		static Real CalculateAlignmentOffset( Alignment alignment, Real contentLen, Real windowLen );
		void UpdateOffsets( S32 renderedContentWidth, S32 renderedContentHeight );

		virtual void Reshape( S32 contentWidth, S32 contentHeight );

	protected:
		bool UpdateContentOrientation( DeviceOrientation::Type newOrientation );

		virtual void SetContentOrientation( DeviceOrientation::Type newOrientation );
#ifdef RTT_SURFACE_ROTATION
		virtual void SetOrientation( DeviceOrientation::Type newOrientation, bool autoRotate );
#endif

	public:
#ifdef RTT_SURFACE_ROTATION
		// Returns relative orientation between content and surface factoring in
		// any modelview fRotation angle
		virtual DeviceOrientation::Type GetRelativeOrientation() const;
#endif

	protected:
		void PushMaskInternal( const BitmapPaint *mask, bool forceTextureCombiner );

	public:
		virtual void SetTextureCoordinates( const Vertex2 *coords, S32 numCoords );
		virtual int GetTextureDepth() const;
		int GetActiveTextureCount() const { return fTextureStackNumActiveFrames; }
		int GetActiveTextureIndex() const { return fTextureStackNumActiveFrames - 1; }

	public:
		virtual void SetClearColor( const Paint& paint );
		virtual RGBA GetClearColor();

	protected:
		const BitmapPaint* GetBitmap() const;

	public:
		virtual U8 GetAlpha() const;
		virtual U8 SetAlpha( U8 newValue, bool accumuluate );

		virtual void CaptureFrameBuffer( BufferBitmap& outBuffer, S32 x, S32 y, S32 w, S32 h );

	public:
		virtual S32 GetRelativeRotation(){return fRotation;}
	

	protected:
		void DrawQuad( const Quad vertices );

	protected:
		const Paint* fCurrentPaint;
		const Vertex2 *fTextureCoords;
		S32 fNumTextureCoords;
		S32 fRotation; // Only used when Rtt_AUTHORING_SIMULATOR is defined
		bool fIsTexture;
		U8 fAlpha;
		U8 fAutoRotate;
		U8 fTextureStackSize;

	private:
		struct TextureStackFrame
		{
			const BitmapPaint *paint;
			int depth;
			bool requiresCombiner;
		};
		int fTextureStackDepth;
		int fTextureStackNumActiveFrames;
		TextureStackFrame fTextureStack[kMaxTextureStackDepth];

	private:
		GLint fWindowWidth;
		GLint fWindowHeight;
		S32 fRenderedContentWidth; // width of rect in which content is rendered (not necessarily same as content width)
		S32 fRenderedContentHeight; // height of rect in which content is rendered (not necessarily same as content height)

		// Frustum
		S32 fOrientedContentWidth; // Current width of the surface (can change due to orientation)
		S32 fOrientedContentHeight; // Current height of the surface (can change due to orientation)
		Real fNear;
		Real fFar;

		// Modelview
		Real fXVertexOffset;
		Real fYVertexOffset;

		// Current state
		RGBA fColor;
		TextureFunction fTextureFunction;

		// Clear color
		GLclampf fClearR;
		GLclampf fClearG;
		GLclampf fClearB;
		GLclampf fClearA;
		
	protected:
		Rtt_Allocator* fAllocator;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_GPUStream_H__
