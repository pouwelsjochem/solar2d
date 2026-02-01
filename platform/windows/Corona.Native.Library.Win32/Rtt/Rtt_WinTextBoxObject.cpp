//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Rtt_WinTextBoxObject.h"
#include "Core\Rtt_Build.h"
#include "Corona\CoronaLua.h"
#include "Display\Rtt_Display.h"
#include "Display\Rtt_LuaLibDisplay.h"
#include "Interop\UI\RenderSurfaceControl.h"
#include "Interop\UI\TextBox.h"
#include "Interop\UI\Window.h"
#include "Interop\RuntimeEnvironment.h"
#include "Rtt_Event.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaLibNative.h"
#include "Rtt_LuaProxy.h"
#include "Rtt_LuaProxyVTable.h"
#include "Rtt_Runtime.h"
#include "Rtt_String.h"
#include "Rtt_WinPlatform.h"
#include "WinString.h"
#include <string>


namespace Rtt
{

#pragma region Constructors/Destructors
WinTextBoxObject::WinTextBoxObject(Interop::RuntimeEnvironment& environment, const Rect& bounds)
:	Super(environment, bounds),
	fTextBoxPointer(nullptr),
	fIsPlaceholderTextNil(true),
	fResizedEventHandler(this, &WinTextBoxObject::OnResized),
	fGainedFocusEventHandler(this, &WinTextBoxObject::OnGainedFocus),
	fLostFocusEventHandler(this, &WinTextBoxObject::OnLostFocus),
	fTextChangedEventHandler(this, &WinTextBoxObject::OnTextChanged),
	fPressedEnterKeyEventHandler(this, &WinTextBoxObject::OnPressedEnterKey),
	fReceivedMessageEventHandler(this, &WinTextBoxObject::OnReceivedMessage)
{
}

WinTextBoxObject::~WinTextBoxObject()
{
	if (fTextBoxPointer)
	{
		delete fTextBoxPointer;
		fTextBoxPointer = nullptr;
	}
}

#pragma endregion


#pragma region Public Methods
Interop::UI::Control* WinTextBoxObject::GetControl() const
{
	return fTextBoxPointer;
}

bool WinTextBoxObject::Initialize()
{
	// Do not continue if this object was already initialized.
	if (fTextBoxPointer)
	{
		return true;
	}

	// Fetch the bounds of this display object converted from Corona coordinates to native screen coordinates.
	Rect screenBounds;
	GetScreenBounds(screenBounds);

	// Create and configure the native Win32 text field this object will manage.
	Interop::UI::TextBox::CreationSettings settings{};
	settings.ParentWindowHandle = GetRuntimeEnvironment().GetRenderSurface()->GetWindowHandle();
	settings.Bounds.left = (LONG)Rtt_RealToInt(screenBounds.xMin);
	settings.Bounds.top = (LONG)Rtt_RealToInt(screenBounds.yMin);
	settings.Bounds.right = (LONG)Rtt_RealToInt(screenBounds.xMax);
	settings.Bounds.bottom = (LONG)Rtt_RealToInt(screenBounds.yMax);
	fTextBoxPointer = new Interop::UI::TextBox(settings);

	// Add event handlers to the native text field.
	fTextBoxPointer->GetResizedEventHandlers().Add(&fResizedEventHandler);
	fTextBoxPointer->GetGainedFocusEventHandlers().Add(&fGainedFocusEventHandler);
	fTextBoxPointer->GetLostFocusEventHandlers().Add(&fLostFocusEventHandler);
	fTextBoxPointer->GetTextChangedEventHandlers().Add(&fTextChangedEventHandler);
	fTextBoxPointer->GetPressedEnterKeyEventHandlers().Add(&fPressedEnterKeyEventHandler);
	fTextBoxPointer->GetReceivedMessageEventHandlers().Add(&fReceivedMessageEventHandler);

	// Let the base class finish initialization of this object.
	return WinDisplayObject::Initialize();
}

const LuaProxyVTable& WinTextBoxObject::ProxyVTable() const
{
	return PlatformDisplayObject::GetTextFieldObjectProxyVTable();
}

int WinTextBoxObject::ValueForKey(lua_State *L, const char key[]) const
{
	// Validate.
	if (!fTextBoxPointer || Rtt_StringIsEmpty(key))
	{
		if (key && !strcmp(PlatformDisplayObject::kUserInputEvent, key))
		{
			// This property will get queried before Initialize() gets called. Let it no-op.
		}
		else
		{
			// Trigger an assert for everything else.
			Rtt_ASSERT(0);
		}
		return 0;
	}

	int result = 1;
	if (strcmp("text", key) == 0)
	{
		// Fetch the UTF-16 encoded text from the control.
		std::wstring utf16Text;
		fTextBoxPointer->CopyTextTo(utf16Text);

		// Convert to UTF-8 and push to Lua. Make sure we never return nil.
		WinString stringTranscoder(utf16Text.c_str());
		auto utf8Text = stringTranscoder.GetUTF8();
		lua_pushstring(L, utf8Text ? utf8Text : "");
	}
	else if (strcmp("setReturnKey", key) == 0)
	{
		lua_pushcfunction(L, WinTextBoxObject::OnSetReturnKey);
	}
	else if (strcmp("setTextColor", key) == 0)
	{
		lua_pushcfunction(L, WinTextBoxObject::OnSetTextColor);
	}
	else if (strcmp("setSelection", key) == 0)
	{
		lua_pushcfunction(L, WinTextBoxObject::OnSetSelection);
	}
	else if (strcmp("align", key) == 0)
	{
		lua_pushstring(L, fTextBoxPointer->GetAlignmentStringId());
	}
	else if (strcmp("isSecure", key) == 0)
	{
		lua_pushboolean(L, fTextBoxPointer->IsSecure() ? 1 : 0);
	}
	else if (strcmp("inputType", key) == 0)
	{
		if (fTextBoxPointer->IsNoEmoji())
		{
			lua_pushstring(L, "no-emoji");
		}
		else if (fTextBoxPointer->IsDecimalNumericOnly())
		{
			lua_pushstring(L, "decimal");
		}
		else if (fTextBoxPointer->IsNumericOnly())
		{
			lua_pushstring(L, "number");
		}
		else
		{
			lua_pushstring(L, "default");
		}
	}
	else if (strcmp("margin", key) == 0)
	{
		auto controlHeight = fTextBoxPointer->GetHeight();
		auto clientHeight = fTextBoxPointer->GetClientHeight();
		auto margin = (double)(controlHeight - clientHeight) / 2.0;
		margin++;
		margin *= (double)Rtt_RealToFloat(GetRuntimeEnvironment().GetRuntime()->GetDisplay().GetScreenToContentScale());
		lua_pushnumber(L, margin);
	}
	else if (strcmp("placeholder", key) == 0)
	{
		// Fetch the UTF-16 encoded placeholder text from the control.
		std::wstring utf16Text;
		fTextBoxPointer->CopyPlaceholderTextTo(utf16Text);

		// Convert to UTF-8 and push to Lua.
		WinString stringTranscoder(utf16Text.c_str());
		auto utf8Text = stringTranscoder.GetUTF8();
		if (fIsPlaceholderTextNil && Rtt_StringIsEmpty(utf8Text))
		{
			lua_pushnil(L);
		}
		else
		{
			lua_pushstring(L, utf8Text);
		}
	}
	else if (!strcmp("autocorrectionType", key) || !strcmp("spellCheckingType", key))
	{
		CoronaLuaWarning(L, "Native TextFields on Windows do not support the \"%s\" property.", key);
		lua_pushnil(L);
	}
	else
	{
		result = Super::ValueForKey(L, key);
	}

	return result;
}

bool WinTextBoxObject::SetValueForKey(lua_State *L, const char key[], int valueIndex)
{
	// Validate.
	if (!fTextBoxPointer || Rtt_StringIsEmpty(key))
	{
		Rtt_ASSERT(0);
		return 0;
	}

	bool result = true;
	if (strcmp("text", key) == 0)
	{
		if (lua_type(L, valueIndex) == LUA_TSTRING)
		{
			WinString stringTranscoder;
			stringTranscoder.SetUTF8(lua_tostring(L, valueIndex));
			fTextBoxPointer->SetText(stringTranscoder.GetUTF16());
		}
		else
		{
			CoronaLuaError(L, "Invalid value type was assigned to the native TextField.%s property.", key);
		}
	}
	else if (strcmp("isSecure", key) == 0)
	{
		if (lua_type(L, valueIndex) == LUA_TBOOLEAN)
		{
			bool isSecure = lua_toboolean(L, valueIndex) ? true : false;
			fTextBoxPointer->SetSecure(isSecure);
		}
		else
		{
			CoronaLuaError(L, "Invalid value type was assigned to the native TextField.%s property.", key);
		}
	}
	else if (strcmp("align", key) == 0)
	{
		if (lua_type(L, valueIndex) == LUA_TSTRING)
		{
			const char* alignmentStringId = lua_tostring(L, valueIndex);
			fTextBoxPointer->SetAlignment(alignmentStringId);
		}
		else
		{
			CoronaLuaError(L, "Invalid value type was assigned to the native TextField.%s property.", key);
		}
	}
	else if (strcmp("inputType", key) == 0)
	{
		if (lua_type(L, valueIndex) == LUA_TSTRING)
		{
			auto inputTypeStringId = lua_tostring(L, valueIndex);
			if (Rtt_StringCompare("default", inputTypeStringId) == 0)
			{
				fTextBoxPointer->SetNumericOnly(false);
				fTextBoxPointer->SetDecimalNumericOnly(false);
				fTextBoxPointer->SetNoEmoji(false);
			}
			else if (Rtt_StringCompare("number", inputTypeStringId) == 0)
			{
				fTextBoxPointer->SetNumericOnly(true);
				fTextBoxPointer->SetDecimalNumericOnly(false);
				fTextBoxPointer->SetNoEmoji(false);
			}
			else if (Rtt_StringCompare("decimal", inputTypeStringId) == 0)
			{
				fTextBoxPointer->SetNumericOnly(false);
				fTextBoxPointer->SetDecimalNumericOnly(true);
				fTextBoxPointer->SetNoEmoji(false);
			}
			else if (Rtt_StringCompare("no-emoji", inputTypeStringId) == 0)
			{
				fTextBoxPointer->SetNumericOnly(false);
				fTextBoxPointer->SetDecimalNumericOnly(false);
				fTextBoxPointer->SetNoEmoji(true);
			}
			else if (inputTypeStringId)
			{
				CoronaLuaWarning(L, "Native TextField.%s key \"%s\" is not supported on Windows.", key, inputTypeStringId);
			}
		}
		else
		{
			CoronaLuaError(L, "Invalid value type was assigned to the native TextField.%s property.", key);
		}
	}
	else if (strcmp("placeholder", key) == 0)
	{
		if (lua_type(L, valueIndex) == LUA_TSTRING)
		{
			WinString stringTranscoder;
			stringTranscoder.SetUTF8(lua_tostring(L, valueIndex));
			fTextBoxPointer->SetPlaceholderText(stringTranscoder.GetUTF16());
			fIsPlaceholderTextNil = false;
		}
		else if (lua_isnil(L, valueIndex))
		{
			fTextBoxPointer->SetPlaceholderText(nullptr);
			fIsPlaceholderTextNil = true;
		}
		else
		{
			CoronaLuaError(L, "Invalid value type was assigned to the native TextField.%s property.", key);
		}
	}
	else if (!strcmp("autocorrectionType", key) || !strcmp("spellCheckingType", key))
	{
		CoronaLuaWarning(L, "Native TextFields on Windows do not support the \"%s\" property.", key);
	}
	else
	{
		result = Super::SetValueForKey(L, key, valueIndex);
	}

	return result;
}

#pragma endregion


#pragma region Private Static Functions
void WinTextBoxObject::OnResized(Interop::UI::Control& sender, const Interop::EventArgs& arguments)
{
	// No-op: Corona drives sizing; native resize events don't need to adjust content bounds.
	(void)sender;
	(void)arguments;
}

void WinTextBoxObject::OnGainedFocus(Interop::UI::Control& sender, const Interop::EventArgs& arguments)
{
	// Dispatch a Lua "userInput" event with phase "began".
	(void)sender;
	(void)arguments;
	Rtt::UserInputEvent event(Rtt::UserInputEvent::kBegan);
	DispatchEventWithTarget(event);
}

void WinTextBoxObject::OnLostFocus(Interop::UI::Control& sender, const Interop::EventArgs& arguments)
{
	// Dispatch a Lua "userInput" event with phase "ended".
	(void)sender;
	(void)arguments;
	Rtt::UserInputEvent event(Rtt::UserInputEvent::kEnded);
	DispatchEventWithTarget(event);
}

void WinTextBoxObject::OnTextChanged(
	Interop::UI::TextBox& sender, const Interop::UI::UITextChangedEventArgs& arguments)
{
	// Create a string providing the characters added, if applicable.
	(void)sender;
	std::string addedString("");
	if (arguments.GetAddedCharacterCount() > 0)
	{
		auto newTextLength = (int)strlen(arguments.GetNewTextAsUtf8());
		if (arguments.GetPreviousStartSelectionIndex() < newTextLength)
		{
			addedString = arguments.GetNewTextAsUtf8() + arguments.GetPreviousStartSelectionIndex();
			if (arguments.GetAddedCharacterCount() < (int)addedString.length())
			{
				addedString.erase(arguments.GetAddedCharacterCount());
			}
		}
	}

	// Dispatch a Lua "userInput" event with phase "editing".
	Rtt::UserInputEvent event(
			arguments.GetPreviousStartSelectionIndex(), arguments.GetDeletedCharacterCount(),
			addedString.c_str(), arguments.GetPreviousTextAsUtf8(), arguments.GetNewTextAsUtf8());
	DispatchEventWithTarget(event);
}

void WinTextBoxObject::OnPressedEnterKey(Interop::UI::TextBox& sender, Interop::HandledEventArgs& arguments)
{
	// Dispatch a Lua "userInput" event with phase "submitted".
	(void)sender;
	Rtt::UserInputEvent event(Rtt::UserInputEvent::kSubmitted);
	DispatchEventWithTarget(event);

	// Flag the enter key as handled.
	arguments.SetHandled();
}

void WinTextBoxObject::OnReceivedMessage(
	Interop::UI::UIComponent& sender, Interop::UI::HandleMessageEventArgs& arguments)
{
	(void)sender;

	if (arguments.WasHandled())
	{
		return;
	}

	UINT messageId = arguments.GetMessageId();
	if (messageId != WM_KEYDOWN)
	{
		return;
	}

	if (arguments.GetWParam() != VK_ESCAPE)
	{
		return;
	}

	Rtt::UserInputEvent event(Rtt::UserInputEvent::kCancelled);
	DispatchEventWithTarget(event);
}

#pragma endregion


#pragma region Private Static Functions
int WinTextBoxObject::OnSetTextColor(lua_State *L)
{
	auto displayObjectPointer = (WinTextBoxObject*)LuaProxy::GetProxyableObject(L, 1);
	if (&displayObjectPointer->ProxyVTable() == &PlatformDisplayObject::GetTextFieldObjectProxyVTable())
	{
		ColorUnion colorConverter;
		colorConverter.pixel = LuaLibDisplay::toColor(L, 2);
		COLORREF nativeColor = RGB(colorConverter.rgba.r, colorConverter.rgba.g, colorConverter.rgba.b);
		if (displayObjectPointer->fTextBoxPointer)
		{
			displayObjectPointer->fTextBoxPointer->SetTextColor(nativeColor);
		}
	}
	return 0;
}

int WinTextBoxObject::OnSetReturnKey(lua_State *L)
{
	(void)L;
	return 0;
}

int WinTextBoxObject::OnSetSelection(lua_State *L)
{
	auto displayObjectPointer = (WinTextBoxObject*)LuaProxy::GetProxyableObject(L, 1);
	if (&displayObjectPointer->ProxyVTable() == &PlatformDisplayObject::GetTextFieldObjectProxyVTable())
	{
		if ((lua_type(L, 2) == LUA_TNUMBER) && (lua_type(L, 3) == LUA_TNUMBER))
		{
			int startIndex = (int)lua_tointeger(L, 2);
			int endIndex = (int)lua_tointeger(L, 3);
			if (startIndex < 0)
			{
				startIndex = 0;
			}
			if (endIndex < 0)
			{
				endIndex = 0;
			}
			displayObjectPointer->fTextBoxPointer->SetSelection(startIndex, endIndex);
		}
	}
	return 0;
}

#pragma endregion


} // namespace Rtt
