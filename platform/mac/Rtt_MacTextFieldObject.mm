//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#import "Rtt_AppleTextAlignment.h"

#include "Rtt_MacTextFieldObject.h"

#import "AppDelegate.h"
#import "Rtt_AppleTextDelegateWrapperObjectHelper.h"

#import <AppKit/NSApplication.h>
#import <AppKit/NSColor.h>

#include "Rtt_Lua.h"
#include "CoronaLua.h"
#include "Rtt_LuaProxy.h"
#include "Rtt_LuaProxyVTable.h"
#include "Rtt_Runtime.h"
#include "Display/Rtt_Display.h"
#include "Display/Rtt_LuaLibDisplay.h"
#include "Display/Rtt_StageObject.h"


// ----------------------------------------------------------------------------

static void
DispatchEvent( Rtt_NSTextField *textField, Rtt::UserInputEvent::Phase phase )
{
	using namespace Rtt;

	MacTextFieldObject *o = ((Rtt_NSTextField*)textField).owner;
	if ( o )
	{
		UserInputEvent e( phase );
		o->DispatchEventWithTarget( e );
	}
}

static void
DispatchEvent( Rtt_NSSecureTextField *textField, Rtt::UserInputEvent::Phase phase )
{
	using namespace Rtt;

	MacTextFieldObject *o = ((Rtt_NSSecureTextField*)textField).owner;
	if ( o )
	{
		UserInputEvent e( phase );
		o->DispatchEventWithTarget( e );
	}
}

static void
DispatchEvent( Rtt_NSTextField *textField, int startpos, int numdeleted, const char* replacementstring, const char* oldstring, const char* newstring )
{
	using namespace Rtt;

	MacTextFieldObject *o = ((Rtt_NSTextField*)textField).owner;
	if ( o )
	{
		UserInputEvent e( startpos, numdeleted, replacementstring, oldstring, newstring );
		o->DispatchEventWithTarget( e );
	}
}

static void
DispatchEvent( Rtt_NSSecureTextField *textField, int startpos, int numdeleted, const char* replacementstring, const char* oldstring, const char* newstring )
{
	using namespace Rtt;

	MacTextFieldObject *o = ((Rtt_NSSecureTextField*)textField).owner;
	if ( o )
	{
		UserInputEvent e( startpos, numdeleted, replacementstring, oldstring, newstring );
		o->DispatchEventWithTarget( e );
	}
}

static void CopyNSTextFieldProperties(NSTextField* source, NSTextField* destination)
{
	[destination setFrame:[source frame]];
	[destination setStringValue:[source stringValue]];
	[destination setFont:[source font]];
	[destination setAlignment:[source alignment]];
	[destination setTextColor:[source textColor]];
	[destination setBezeled:[source isBezeled]];
	[destination setBordered:[source isBordered]];
	[destination setDrawsBackground:[source drawsBackground]];
	[destination.cell setWraps:[source.cell wraps]];
	[destination.cell setScrollable:[source.cell isScrollable]];
}

@implementation Rtt_NSTextField

@synthesize owner;

// For some reason overriding drawRect: but merely calling super is enough to make displaying transparent
// NSTextFields work (without this, fields with no background display a black rectangle)
- (void)drawRect:(NSRect)dirtyRect
{
	[super drawRect:dirtyRect];
}

// Instead of sending began on the start of typing, we now send it when the object becomes the first responder to emulate iOS a little more closely.
- (BOOL) becomeFirstResponder;
{
	BOOL did_become_first_responder = [super becomeFirstResponder];
	if( YES == did_become_first_responder )
	{
		// Don't cache the value of [self delegate] because it might get removed by an event
		if( nil != [self delegate] )
		{
			DispatchEvent( self, Rtt::UserInputEvent::kBegan );
		}
	}
	return did_become_first_responder;
}

- (void) controlTextDidEndEditing:(NSNotification*)notification
{
	if( nil != [self delegate] )
	{
		if ( [[[notification userInfo] objectForKey:@"NSTextMovement"] intValue] == NSReturnTextMovement )
		{
			DispatchEvent( self, Rtt::UserInputEvent::kSubmitted );			
		}
		else
		{
			DispatchEvent( self, Rtt::UserInputEvent::kEnded );
		}
	}
}

- (BOOL)textView:(NSTextView*)textView shouldChangeTextInRange:(NSRange)range replacementString:(NSString*)string
{
	// location (first number) is the start position of the string
	// length (second number) is the length or number of the characters replaced/deleted
	//	NSLog(@"%@, %@, %@", NSStringFromSelector(_cmd), NSStringFromRange(range), string );

	if (owner->rejectDisallowedCharacters([string UTF8String]))
	{
		return NO;
	}

	// Must release object on other side of performSelector
	Rtt_AppleTextDelegateWrapperObjectHelper* eventData = [[Rtt_AppleTextDelegateWrapperObjectHelper alloc] init];
	eventData.textWidget = self;
	eventData.theRange = range;
	eventData.replacementString = string;
	
	[self performSelector:@selector(dispatchEditingEventForTextField:) withObject:eventData afterDelay:0.0];
	
	return YES;
}

- (void) dispatchEditingEventForTextField:(Rtt_AppleTextDelegateWrapperObjectHelper*)eventData
{
	Rtt_NSTextField* textfield = (Rtt_NSTextField*)eventData.textWidget;
	NSRange range = eventData.theRange;
	NSString* string = eventData.replacementString;
	NSString* oldstring = eventData.originalString;

	// Don't cache the value of [self delegate] because it might get removed by an event
	if( nil != [self delegate] )
	{
		// Don't forget to add 1 for Lua index conventions
		DispatchEvent( textfield, (int) range.location+1, (int) range.length, [string UTF8String], [oldstring UTF8String], [[textfield stringValue] UTF8String] );
	}
	
	// Data must be released or it will leak because it created so it could be passed through this callback.
	[eventData release];
}

@end

@implementation Rtt_NSSecureTextField

@synthesize owner;

- (BOOL) becomeFirstResponder;
{
	BOOL did_become_first_responder = [super becomeFirstResponder];

	if( YES == did_become_first_responder )
	{
		// Don't cache the value of [self delegate] because it might get removed by an event
		if( nil != [self delegate] )
		{
			DispatchEvent( self, Rtt::UserInputEvent::kBegan );
		}
	}

	return did_become_first_responder;
}

- (void) controlTextDidEndEditing:(NSNotification*)notification
{
	if( nil != [self delegate] )
	{
		// If the user hit Return, we send the "submitted" event.
		// When the field loses focus, we send the "ended" event.
		if ( [[[notification userInfo] objectForKey:@"NSTextMovement"] intValue] == NSReturnTextMovement )
		{
			DispatchEvent( self, Rtt::UserInputEvent::kSubmitted );
		}
		else
		{
			DispatchEvent( self, Rtt::UserInputEvent::kEnded );
		}
	}
}

- (BOOL)textView:(NSTextView*)textView shouldChangeTextInRange:(NSRange)range replacementString:(NSString*)string
{
	// location (first number) is the start position of the string
	// length (second number) is the length or number of the characters replaced/deleted
	//	NSLog(@"%@, %@, %@", NSStringFromSelector(_cmd), NSStringFromRange(range), string );
	
	// Must release object on other side of performSelector
	Rtt_AppleTextDelegateWrapperObjectHelper* eventData = [[Rtt_AppleTextDelegateWrapperObjectHelper alloc] init];
	eventData.textWidget = self;
	eventData.theRange = range;
	eventData.replacementString = string;
	
	[self performSelector:@selector(dispatchEditingEventForTextField:) withObject:eventData afterDelay:0.0];
	
	return YES;
}

- (void) dispatchEditingEventForTextField:(Rtt_AppleTextDelegateWrapperObjectHelper*)eventData
{
	Rtt_NSSecureTextField* textfield = (Rtt_NSSecureTextField*)eventData.textWidget;
	NSRange range = eventData.theRange;
	NSString* string = eventData.replacementString;
	NSString* oldstring = eventData.originalString;
	
	if( nil != [self delegate] )
	{
		// Don't forget to add 1 for Lua index conventions
		DispatchEvent( textfield, (int) range.location+1, (int) range.length, [string UTF8String], [oldstring UTF8String], [[textfield stringValue] UTF8String] );
	}
	
	// Data must be released or it will leak because it created so it could be passed through this callback.
	[eventData release];
}

@end

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

MacTextFieldObject::MacTextFieldObject( const Rect& bounds )
:	Super( bounds ),
	fNoEmoji( false ),
	fNumbersOnly( false ),
	fDecimalNumbersOnly( false )
{
}

MacTextFieldObject::~MacTextFieldObject()
{
	// Nil out the delegate to prevent any notifications from being triggered on this dead object
	NSTextField* textfield = (NSTextField*)GetView();
	[textfield setDelegate:nil];
}

bool
MacTextFieldObject::Initialize()
{
	Rtt_ASSERT( ! GetView() );

	Rect screenBounds;
	GetScreenBounds( screenBounds );

	NSRect r = NSMakeRect( screenBounds.xMin, screenBounds.yMin, screenBounds.Width(), screenBounds.Height() );

	Rtt_NSTextField *t = [[Rtt_NSTextField alloc] initWithFrame:r];

	t.delegate = (id<NSTextFieldDelegate>)t;
	t.owner = this;
	
	// We have a problem because  Build(matrix) is computed some time after the object was created.
	// If we insert the object when created, when this method is invoked shortly later, you can see
	// a visible jump in the object's position which looks bad.
	// The workaround is to defer adding the object to the view until after this method has been computed at least once.
	Super::InitializeView( t );
	[t release];
	
	// Set NSTextField defaults we want
	[(NSTextField*)GetView() setBezeled:NO];
	[(NSTextField*)GetView() setBordered:NO];
	[(NSTextField*)GetView() setDrawsBackground:YES];
	// Setting these reproduces device behavior much better than setting setUsesSingleLineMode
	[[(NSTextField*)GetView() cell] setWraps:NO];
	[[(NSTextField*)GetView() cell] setScrollable:YES];
	
	return (t != nil);
}

const LuaProxyVTable&
MacTextFieldObject::ProxyVTable() const
{
	return PlatformDisplayObject::GetTextFieldObjectProxyVTable();
}


int
MacTextFieldObject::setTextColor( lua_State *L )
{
	PlatformDisplayObject *o = (PlatformDisplayObject*)LuaProxy::GetProxyableObject( L, 1 );
	if ( & o->ProxyVTable() == & PlatformDisplayObject::GetTextFieldObjectProxyVTable() )
	{
		ColorUnion colorConverter;
		colorConverter.pixel = LuaLibDisplay::toColor( L, 2 );
		NSColor *c = [NSColor colorWithCalibratedRed:(colorConverter.rgba.r / 255.0f)
											 green:(colorConverter.rgba.g / 255.0f)
											  blue:(colorConverter.rgba.b / 255.0f)
											 alpha:(colorConverter.rgba.a / 255.0f)];
		NSView *v = ((MacTextFieldObject*)o)->GetView();
		Rtt_NSTextField *t = (Rtt_NSTextField*)v;
		t.textColor = c;
	}
	return 0;
}

int
MacTextFieldObject::setSelection( lua_State *L )
{
	(void)L;
	Rtt_TRACE_SIM( ( "WARNING: setSelection is not supported by the simulator on TextField objects.\n" ) );
	return 0;
}
	
// TODO: move these somewhere in librtt, so all platforms use same constants
static const char kDefaultInputType[] = "default";

int
MacTextFieldObject::ValueForKey( lua_State *L, const char key[] ) const
{
	Rtt_ASSERT( key );

	int result = 1;
	
	NSTextField* textfield = (NSTextField*)GetView();

	if ( strcmp( "text", key ) == 0 )
	{
		if ( nil == [textfield stringValue] )
		{
			lua_pushstring( L, "" );
		}
		else
		{
			lua_pushstring( L, [[textfield stringValue] UTF8String] );
		}
	}
	else if ( 0 == strcmp( key, "placeholder" ) )
	{
		NSString *text = [[textfield cell] placeholderString];
		if ( text )
		{
			lua_pushstring( L, [text UTF8String] );
		}
		else
		{
			lua_pushnil( L );
		}
	}
	else if ( strcmp( "isSecure", key ) == 0 )
	{
		lua_pushboolean( L, [textfield isKindOfClass:[NSSecureTextField class]] );
	}
	else if ( strcmp( "align", key ) == 0 )
	{
		lua_pushstring( L, AppleAlignment::StringForAlignment( [textfield alignment] ) );
	}
	else if ( strcmp( "setTextColor", key ) == 0 )
	{
		lua_pushcfunction( L, setTextColor );
	}
	else if ( strcmp( "setReturnKey", key ) == 0 )
	{
		lua_pushcfunction( L, setReturnKey);
	}
	else if ( strcmp( "setSelection", key ) == 0 )
	{
		lua_pushcfunction( L, setSelection);
	}
	else if ( strcmp( "inputType", key ) == 0 )
	{
		const char *value = kDefaultInputType;
		if ( fNoEmoji )
		{
			value = "no-emoji";
		}
		else if ( fDecimalNumbersOnly )
		{
			value = "decimal";
		}
		else if ( fNumbersOnly )
		{
			value = "number";
		}
		lua_pushstring( L, value );
	}
	else if ( strcmp( "hasBackground", key ) == 0 )
	{
		lua_pushboolean( L, [textfield drawsBackground]);
	}
	else if ( strcmp( "margin", key ) == 0 )
	{
		// The margin/padding between the edge of this field and its text in content coordinates.
		float value = 2.0f * GetStage()->GetDisplay().GetScreenToContentScale();
		lua_pushnumber( L, value );
	}
	else
	{
		result = Super::ValueForKey( L, key );
	}

	return result;
}

bool
MacTextFieldObject::SetValueForKey( lua_State *L, const char key[], int valueIndex )
{
	Rtt_ASSERT( key );

	bool result = true;

	NSTextField* textfield = (NSTextField*)GetView();

	if ( strcmp( "text", key ) == 0 )
	{
		const char *s = lua_tostring( L, valueIndex );
		if ( Rtt_VERIFY( s ) )
		{
			NSString *newValue = [[NSString alloc] initWithUTF8String:s];
			[textfield setStringValue:newValue];
			[newValue release];
		}
	}
	else if ( 0 == strcmp( key, "placeholder" ) )
	{
		const char *s = lua_tostring( L, valueIndex );
		if ( s )
		{
			[[textfield cell] setPlaceholderString:[NSString stringWithExternalString:s]];
		}
		else
		{
			[[textfield cell] setPlaceholderString:nil];
		}
	}
	else if ( strcmp( "isSecure", key ) == 0 )
	{
		if ( ! lua_isboolean(L, valueIndex) )
		{
			CoronaLuaWarning(L, "TextField.isSecure should be a boolean (not a %s)", lua_typename(L, lua_type(L, valueIndex)));
		}

		BOOL is_secure = lua_toboolean( L, valueIndex );

		// We must create a different object: NSSecureTextField and somehow swap it behind the scenes.
		// Only do this if the flag has changed.
		if( [textfield isKindOfClass:[NSSecureTextField class]] != is_secure )
		{
			NSTextField* new_field = nil;
			if(YES == is_secure)
			{
				new_field = [[Rtt_NSSecureTextField alloc] initWithFrame:[textfield frame]];
				((Rtt_NSSecureTextField*)new_field).owner = this;
			}
			else
			{
				new_field = [[Rtt_NSTextField alloc] initWithFrame:[textfield frame]];
				((Rtt_NSTextField*)new_field).owner = this;

			}
			new_field.delegate = (id<NSTextFieldDelegate>)new_field;
			
			CopyNSTextFieldProperties(textfield, new_field);
			
			// update fView pointer
			[fView autorelease];
			fView = [new_field retain];

			// Add only to super if already in the scene
			if ( nil != [textfield superview] )
			{
				[textfield removeFromSuperview];
				AddSubviewToLayerHostView();
			}
			
			[new_field release];
		}
	}
	else if ( strcmp( "align", key ) == 0 )
	{
		[textfield setAlignment:AppleAlignment::AlignmentForString( lua_tostring( L, valueIndex ) )];
	}
	else if ( strcmp( "inputType", key ) == 0 )
	{
		const char *s = lua_tostring( L, valueIndex );
		fNoEmoji = false;
		fNumbersOnly = false;
		fDecimalNumbersOnly = false;
		if ( s != NULL )
		{
			if (strcmp(s, "no-emoji") == 0)
			{
				fNoEmoji = true;
			}
			else if (strcmp(s, "number") == 0)
			{
				fNumbersOnly = true;
			}
			else if (strcmp(s, "decimal") == 0)
			{
				fDecimalNumbersOnly = true;
			}
		}
	}
	else if ( strcmp( "hasBackground", key ) == 0 )
	{
		if ( ! lua_isboolean(L, valueIndex) )
		{
			CoronaLuaWarning(L, "TextField.hasBackground should be a boolean (not a %s)", lua_typename(L, lua_type(L, valueIndex)));
		}

		BOOL hasBackground = (BOOL)lua_toboolean( L, valueIndex );

		[textfield setDrawsBackground:hasBackground];
	}
	else
	{
		result = Super::SetValueForKey( L, key, valueIndex );
	}

	return result;

}

bool
MacTextFieldObject::rejectDisallowedCharacters(const char *str)
{
	if (str == NULL || str[0] == '\0')
	{
		return false;
	}

	if (fNoEmoji)
	{
		// Reject emojis (or more precisely Unicode characters with the "Symbol Other" property; this matches the behavior on Android)
		NSError *error = NULL;
		static NSRegularExpression *emojiRegex = [[NSRegularExpression regularExpressionWithPattern:@"\\p{So}"
																							options:0
																							  error:&error] retain];

		NSString *string = [NSString stringWithUTF8String:str];
		return ([emojiRegex numberOfMatchesInString:string options:0 range:NSMakeRange(0, [string length])] > 0);
	}
	else if (fNumbersOnly)
	{
		NSError *error = NULL;
		static NSRegularExpression *numbersRegex = [[NSRegularExpression regularExpressionWithPattern:@"[^0-9]"
																							options:0
																							  error:&error] retain];

		NSString *string = [NSString stringWithUTF8String:str];
		return ([numbersRegex numberOfMatchesInString:string options:0 range:NSMakeRange(0, [string length])] > 0);
	}
	else if (fDecimalNumbersOnly)
	{
		NSError *error = NULL;
		static NSString *decimalSeparator = [[NSLocale currentLocale] decimalSeparator];
		static NSString *decimalNumberPattern = [@"[^0-9.]" stringByReplacingOccurrencesOfString:@"." withString:decimalSeparator];
		static NSRegularExpression *decimalNumberRegex = [[NSRegularExpression regularExpressionWithPattern:decimalNumberPattern
																							  options:0
																								error:&error] retain];

		NSString *string = [NSString stringWithUTF8String:str];
		bool reject = [decimalNumberRegex numberOfMatchesInString:string options:0 range:NSMakeRange(0, [string length])] > 0;

		if (! reject)
		{
			NSTextField* textfield = (NSTextField*)GetView();

			// if this character is a period and we already have one, it's rejected
			if ([string isEqualToString:decimalSeparator] && [[textfield stringValue] contains:decimalSeparator])
			{
				reject = true;
			}
		}

		return reject;
	}
	else
	{
		return false;
	}
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------
