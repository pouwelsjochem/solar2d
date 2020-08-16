//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_TextObject_H__
#define _Rtt_TextObject_H__

#include "Core/Rtt_Real.h"
#include "Core/Rtt_String.h"
#include "Display/Rtt_RectObject.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class BitmapPaint;
class Geometry;
class Paint;
class PlatformFont;
class RectPath;
class Runtime;
class Uniform;

// ----------------------------------------------------------------------------

class TextObject : public RectObject
{
	Rtt_CLASS_NO_COPIES( TextObject )

	public:
		typedef RectObject Super;

	public:
		static void Unload( DisplayObject& parent );
		static void Reload( DisplayObject& parent );

	public:
		// TODO: Use a string class instead...
		// TextObject retains ownership of font
		TextObject( Display& display, const char text[], PlatformFont *font, Real w, Real h, const char alignment[] );
		virtual ~TextObject();

	protected:
		bool Initialize();
		void UpdateScaledFont();
		void Reset();

	public:
		void Unload();
		void Reload();

	public:
		// MDrawable
		virtual bool UpdateTransform( const Matrix& parentToDstSpace );
		virtual void GetSelfBounds( Rect& rect ) const;

	public:
		virtual const LuaProxyVTable& ProxyVTable() const;

	public:
		bool IsInitialized() const { return GetMask() ? true : false; }
		
	public:
		// TODO: Text properties (size, font, color, etc).  Ugh!
		void SetColor( Paint* newValue );

		void SetText( const char* newValue );
		const char* GetText() const { return fText.GetString(); }

		Real GetBaselineOffset() const { return fBaselineOffset; }
		void SetSize( Real newValue );
		Real GetSize() const;

		// Note: assumes receiver will own the font after SetFont() is called
		void SetFont( PlatformFont *newValue );
//		const PlatformFont* GetFont() const { return fFont; }

		void SetAlignment( const char* newValue );
		const char* GetAlignment() const { return fAlignment.GetString(); };


	private:
		Display& fDisplay;
		String fText;
		PlatformFont* fOriginalFont;
		PlatformFont* fScaledFont;
		Real fWidth;
		Real fHeight;
		Real fBaselineOffset;	
		String fAlignment;
		mutable Geometry *fGeometry;
		mutable Uniform *fMaskUniform;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_TextObject_H__
