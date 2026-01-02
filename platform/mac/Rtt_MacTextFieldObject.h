//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_MacTextFieldObject_H__
#define _Rtt_MacTextFieldObject_H__

#include "Rtt_MacDisplayObject.h"

#import <AppKit/NSSecureTextField.h>
#import <AppKit/NSTextField.h>
#import <AppKit/NSTextView.h>

// ----------------------------------------------------------------------------

@class Rtt_NSTextField;
@class Rtt_NSSecureTextField;

namespace Rtt
{

// ----------------------------------------------------------------------------

class MacTextFieldObject : public MacDisplayObject
{
	public:
		typedef MacTextFieldObject Self;
		typedef MacDisplayObject Super;

	public:
		MacTextFieldObject( const Rect& bounds );
		virtual ~MacTextFieldObject();

	public:
		// PlatformDisplayObject
		virtual bool Initialize();

	public:
		// DisplayObject
		virtual const LuaProxyVTable& ProxyVTable() const;

	public:
		// MLuaBindable
		virtual int ValueForKey( lua_State *L, const char key[] ) const;
		virtual bool SetValueForKey( lua_State *L, const char key[], int valueIndex );
		bool rejectDisallowedCharacters(const char *str);

	protected:
		static int setTextColor( lua_State *L );
		static int setSelection( lua_State *L );

	private:
		bool fNoEmoji;
		bool fNumbersOnly;
		bool fDecimalNumbersOnly;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

@interface Rtt_NSTextField : NSTextField
{
	Rtt::MacTextFieldObject *owner;
}

@property(nonatomic, assign) Rtt::MacTextFieldObject *owner;

@end


@interface Rtt_NSSecureTextField : NSSecureTextField
{
	Rtt::MacTextFieldObject *owner;
}

@property(nonatomic, assign) Rtt::MacTextFieldObject *owner;

@end

// ----------------------------------------------------------------------------

#endif // _Rtt_MacTextFieldObject_H__
