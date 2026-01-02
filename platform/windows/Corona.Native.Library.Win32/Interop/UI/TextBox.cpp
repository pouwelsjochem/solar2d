//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TextBox.h"
#include "WinString.h"
#include <CommCtrl.h>
#include <cwctype>
#include <cstring>
#include <regex>


namespace Interop { namespace UI {

#pragma region Constructors/Destructors
TextBox::TextBox(const TextBox::CreationSettings& settings)
:	Control(),
	fReceivedMessageEventHandler(this, &TextBox::OnReceivedMessage),
	fIsUsingCustomTextColor(false),
	fCustomTextColor(RGB(0, 0, 0)),
	fLastUpdatedText(L""),
	fIsDecimalNumericOnly(false),
	fNoEmoji(false)
{
	// Set up the text box window styles. Always single line for native text fields.
	DWORD styles = WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	styles |= ES_AUTOHSCROLL;

	// Create the text box child control.
	auto windowHandle = ::CreateWindowExW(
			WS_EX_CLIENTEDGE, L"EDIT", L"", styles,
			settings.Bounds.left, settings.Bounds.top,
			settings.Bounds.right - settings.Bounds.left,
			settings.Bounds.bottom - settings.Bounds.top,
			settings.ParentWindowHandle, nullptr, ::GetModuleHandle(nullptr), nullptr);

	// Use the system's default GUI font for consistent sizing.
	if (windowHandle)
	{
		auto fontHandle = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
		if (fontHandle)
		{
			::SendMessageW(windowHandle, WM_SETFONT, (WPARAM)fontHandle, 0);
		}
	}

	// Store the window handle and start listening to its Windows message events.
	OnSetWindowHandle(windowHandle);

	// Add event handlers.
	GetReceivedMessageEventHandlers().Add(&fReceivedMessageEventHandler);
}

TextBox::~TextBox()
{
	// Fetch the control's window handle.
	auto windowHandle = GetWindowHandle();
	if (!windowHandle)
	{
		return;
	}

	// Remove event handlers.
	GetReceivedMessageEventHandlers().Remove(&fReceivedMessageEventHandler);

	// Detach the WndProc callback from the window.
	// Note: Must be done before destroying it.
	OnSetWindowHandle(nullptr);

	// Destroy the control.
	::DestroyWindow(windowHandle);
}

#pragma endregion


#pragma region Public Methods
TextBox::TextChangedEvent::HandlerManager& TextBox::GetTextChangedEventHandlers()
{
	return fTextChangedEvent.GetHandlerManager();
}

TextBox::PressedEnterKeyEvent::HandlerManager& TextBox::GetPressedEnterKeyEventHandlers()
{
	return fPressedEnterKeyEvent.GetHandlerManager();
}

bool TextBox::IsSecure() const
{
	bool isSecure = false;
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		auto styles = ::GetWindowLongPtrW(windowHandle, GWL_STYLE);
		isSecure = ((styles & ES_PASSWORD) != 0);
	}
	return isSecure;
}

void TextBox::SetSecure(bool value)
{
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		auto styles = ::GetWindowLongPtrW(windowHandle, GWL_STYLE);
		if (value)
		{
			styles |= ES_PASSWORD;
		}
		else
		{
			styles &= ~ES_PASSWORD;
		}
		::SetWindowLongPtrW(windowHandle, GWL_STYLE, styles);
		::SendMessageW(windowHandle, EM_SETPASSWORDCHAR, (WPARAM)(value ? 0x25CF : 0), 0);
		::InvalidateRect(windowHandle, nullptr, FALSE);
	}
}

bool TextBox::IsReadOnly() const
{
	bool isReadOnly = false;
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		auto styles = ::GetWindowLongPtrW(windowHandle, GWL_STYLE);
		isReadOnly = ((styles & ES_READONLY) != 0);
	}
	return isReadOnly;
}

void TextBox::SetReadOnly(bool value)
{
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		::SendMessageW(windowHandle, EM_SETREADONLY, value ? TRUE : FALSE, 0);
		::InvalidateRect(windowHandle, nullptr, FALSE);
	}
}

bool TextBox::IsNumericOnly() const
{
	bool isNumericOnly = false;
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		auto styles = ::GetWindowLongPtrW(windowHandle, GWL_STYLE);
		isNumericOnly = ((styles & ES_NUMBER) != 0);
	}
	return isNumericOnly;
}

void TextBox::SetNumericOnly(bool value)
{
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		auto styles = ::GetWindowLongPtrW(windowHandle, GWL_STYLE);
		if (value)
		{
			styles |= ES_NUMBER;
		}
		else
		{
			styles &= ~ES_NUMBER;
		}
		::SetWindowLongPtrW(windowHandle, GWL_STYLE, styles);
		::InvalidateRect(windowHandle, nullptr, FALSE);
	}
}

bool TextBox::IsDecimalNumericOnly() const
{
	return fIsDecimalNumericOnly;
}

void TextBox::SetDecimalNumericOnly(bool value)
{
	fIsDecimalNumericOnly = value;
}

bool TextBox::IsNoEmoji() const
{
	return fNoEmoji;
}

void TextBox::SetNoEmoji(bool value)
{
	fNoEmoji = value;
}

void TextBox::SetText(const wchar_t* text)
{
	// Fetch the control's window handle.
	auto windowHandle = GetWindowHandle();
	if (!windowHandle)
	{
		return;
	}

	// If given null, then set the text to an empty string.
	if (!text)
	{
		text = L"";
	}

	// Fetch the current text selection positions.
	DWORD startCursorIndex = 0;
	DWORD endCursorIndex = 0;
	::SendMessageW(windowHandle, EM_GETSEL, (WPARAM)&startCursorIndex, (LPARAM)&endCursorIndex);
	auto previousTextLength = ::GetWindowTextLengthW(windowHandle);

	// Update this member variable to prevent a "TextChanged" event from getting raised.
	// A "TextChanged" event should not be raised when changing the text programmatically.
	fLastUpdatedText = text;

	// Update the control's text.
	::SetWindowTextW(windowHandle, text);

	// Update the cursor position after a text update to make it behave like it does on OS X, iOS, and Android.
	auto newTextLength = ::GetWindowTextLengthW(windowHandle);
	if (previousTextLength <= 0)
	{
		// The cursor was at the beginning of the text before. Make sure to keep it there.
		::SendMessageW(windowHandle, EM_SETSEL, 0, 0);
	}
	else if (((int)endCursorIndex >= previousTextLength) || ((int)endCursorIndex >= newTextLength))
	{
		// The cursor was at the end of the text before or its at the end of the new text.
		// In either case, make sure the cursor is at the end of the text now.
		::SendMessageW(windowHandle, EM_SETSEL, (WPARAM)newTextLength, (LPARAM)newTextLength);
	}
	else
	{
		// The cursor was in the middle of the text before. Maintain that same position.
		::SendMessageW(windowHandle, EM_SETSEL, (WPARAM)endCursorIndex, (LPARAM)endCursorIndex);
	}

	// Scroll the current cursor position into view.
	::SendMessageW(windowHandle, EM_SCROLLCARET, 0, 0);
	::InvalidateRect(windowHandle, nullptr, FALSE);
}

bool TextBox::CopyTextTo(std::wstring& text) const
{
	// Fetch the control's window handle.
	auto windowHandle = GetWindowHandle();
	if (!windowHandle)
	{
		return false;
	}

	// Fetch the number of characters in the text field.
	// Note: This does not include the null character.
	auto length = ::GetWindowTextLengthW(windowHandle);

	// Do not continue if the field is empty. (This is an optimization.)
	if (length <= 0)
	{
		text.clear();
		return true;
	}

	// Fetch the field's text and copy it the argument.
	auto utf16Buffer = new wchar_t[length + 1];
	utf16Buffer[0] = L'\0';
	::GetWindowTextW(windowHandle, utf16Buffer, length + 1);
	text = utf16Buffer;
	delete[] utf16Buffer;
	return true;
}

void TextBox::SetPlaceholderText(const wchar_t* text)
{
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		::SendMessageW(windowHandle, EM_SETCUEBANNER, FALSE, (LPARAM)text);
		::InvalidateRect(windowHandle, nullptr, FALSE);
	}
}

bool TextBox::CopyPlaceholderTextTo(std::wstring& text) const
{
	// Fetch the control's window handle.
	auto windowHandle = GetWindowHandle();
	if (!windowHandle)
	{
		return false;
	}

	const size_t kMaxUtf16BufferLength = 512;
	wchar_t utf16Buffer[kMaxUtf16BufferLength];
	utf16Buffer[0] = L'\0';
	::SendMessageW(windowHandle, EM_GETCUEBANNER, (WPARAM)utf16Buffer, kMaxUtf16BufferLength);
	text = utf16Buffer;
	return true;
}

bool TextBox::HasPlaceholderText() const
{
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		wchar_t utf16Buffer[2];
		utf16Buffer[0] = L'\0';
		::SendMessageW(windowHandle, EM_GETCUEBANNER, (WPARAM)utf16Buffer, 2);
		if (utf16Buffer[0] != L'\0')
		{
			return true;
		}
	}
	return false;
}

const char* TextBox::GetAlignmentStringId() const
{
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		auto styles = ::GetWindowLongPtrW(windowHandle, GWL_STYLE);
		if (styles & ES_CENTER)
		{
			return "center";
		}
		if (styles & ES_RIGHT)
		{
			return "right";
		}
	}
	return "left";
}

void TextBox::SetAlignment(const char* alignmentStringId)
{
	auto windowHandle = GetWindowHandle();
	if (!windowHandle)
	{
		return;
	}

	auto styles = ::GetWindowLongPtrW(windowHandle, GWL_STYLE);
	styles &= ~(ES_LEFT | ES_CENTER | ES_RIGHT);
	if (alignmentStringId)
	{
		if (_stricmp(alignmentStringId, "center") == 0)
		{
			styles |= ES_CENTER;
		}
		else if (_stricmp(alignmentStringId, "right") == 0)
		{
			styles |= ES_RIGHT;
		}
		else
		{
			styles |= ES_LEFT;
		}
	}
	else
	{
		styles |= ES_LEFT;
	}
	::SetWindowLongPtrW(windowHandle, GWL_STYLE, styles);
	::InvalidateRect(windowHandle, nullptr, FALSE);
}

void TextBox::SetSelection(int startCharacterIndex, int endCharacterIndex)
{
	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		::SendMessageW(windowHandle, EM_SETSEL, (WPARAM)startCharacterIndex, (LPARAM)endCharacterIndex);
		::InvalidateRect(windowHandle, nullptr, FALSE);
	}
}

void TextBox::SetTextColorToDefault()
{
	fIsUsingCustomTextColor = false;

	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		::InvalidateRect(windowHandle, nullptr, FALSE);
	}
}

void TextBox::SetTextColor(COLORREF value)
{
	fIsUsingCustomTextColor = true;
	fCustomTextColor = value;

	auto windowHandle = GetWindowHandle();
	if (windowHandle)
	{
		::InvalidateRect(windowHandle, nullptr, FALSE);
	}
}

COLORREF TextBox::GetTextColor() const
{
	COLORREF value;
	if (fIsUsingCustomTextColor)
	{
		value = fCustomTextColor;
	}
	else
	{
		value = ::GetSysColor(COLOR_WINDOWTEXT);
	}
	return value;
}

#pragma endregion


#pragma region Private Methods
void TextBox::OnReceivedMessage(UIComponent& sender, HandleMessageEventArgs& arguments)
{
	// Do not continue if the received message was already handled.
	if (arguments.WasHandled())
	{
		return;
	}

	// Handle the message
	switch (arguments.GetMessageId())
	{
		case WM_SETFOCUS:
		{
			// Select the entire field's text when the control gains the focus.
			// This matches Microsoft's UI tabbing behavior. This also matches the behavior on OS X as well.
			SetSelection(0, -1);
			break;
		}
		case WM_CHAR:
		{
			if (VK_RETURN == arguments.GetWParam())
			{
				// Raise a "PressedEnterKey" event.
				HandledEventArgs enterEventArguments;
				fPressedEnterKeyEvent.Raise(*this, enterEventArguments);

				// If the above was handled, then do not let the text box receive the enter key.
				// This will prevent it from making an error sound since it's an invalid key for single line text boxes.
				if (enterEventArguments.WasHandled())
				{
					arguments.SetReturnResult(0);
					arguments.SetHandled();
				}
			}
			else if (fIsDecimalNumericOnly && iswprint(arguments.GetWParam()))
			{
				std::wstring newText;
				CopyTextTo(newText);

				// TODO: localized decimal separator can be up to 3 characters but we only test the first.
				TCHAR szSeparator[4];
				GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szSeparator, 4);

				// If the typed character is not a decimal digit and it's not a decimal separator (or we already have one),
				// ignore the typed character.
				if (!iswdigit(arguments.GetWParam()) &&
					((arguments.GetWParam() != szSeparator[0] || newText.find(szSeparator) != std::wstring::npos)))
				{
					// Ignore the keystroke.
					arguments.SetReturnResult(0);
					arguments.SetHandled();
				}
			}
			else if (fNoEmoji)
			{
				// This captures most emoji (in particular the 8-byte entities that are the root of the problem).
				// TODO: when we have a version of regex on Windows that supports Unicode properties we can switch to the
				// same regex used on other platforms: "\\p{So}".
				std::wregex emoji_regex(L"[\\uD83C-\\uDBFF\\uDC00-\\uDFFF]+");
				std::wstring candidate;

				candidate.push_back(arguments.GetWParam());

				if (std::regex_match(candidate, emoji_regex))
				{
					// Ignore the keystroke.
					arguments.SetReturnResult(0);
					arguments.SetHandled();
				}
			}
			break;
		}
		case WM_REFLECTED(WM_COMMAND):
		{
			switch (HIWORD(arguments.GetWParam()))
			{
				case EN_UPDATE:
				{
					// Determine if the text within the text box has changed.
					std::wstring newText;
					CopyTextTo(newText);
					if (newText != fLastUpdatedText)
					{
						// Create the TextChanged event arguments.
						UITextChangedEventArgs::Settings settings{};
						settings.NewText = newText.c_str();
						settings.PreviousText = fLastUpdatedText.c_str();
						settings.PreviousStartSelectionIndex = 0;
						settings.AddedCharacterCount = (int)newText.length();
						settings.DeletedCharacterCount = (int)fLastUpdatedText.length();
						UITextChangedEventArgs textChangedEventArguments(settings);

						// Store the updated text for the next text change check.
						// Note: We must do this before raising the event below in case it triggers a recursive update.
						fLastUpdatedText = newText;

						// Raise a "ChangedText" event.
						fTextChangedEvent.Raise(*this, textChangedEventArguments);
					}
					break;
				}
			}
			break;
		}
		case WM_REFLECTED(WM_CTLCOLOREDIT):
		case WM_REFLECTED(WM_CTLCOLORSTATIC):
		{
			// Use a custom text color if enabled.
			if (fIsUsingCustomTextColor)
			{
				::SetTextColor((HDC)arguments.GetWParam(), fCustomTextColor);
				arguments.SetReturnResult((LRESULT)::GetStockObject(DC_BRUSH));
				arguments.SetHandled();
			}
			break;
		}
	}
}

#pragma endregion

} }	// namespace Interop::UI
