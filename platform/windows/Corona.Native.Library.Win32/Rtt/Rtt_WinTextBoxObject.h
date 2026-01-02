//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core\Rtt_Build.h"
#include "Interop\UI\TextBox.h"
#include "Renderer\Rtt_RenderData.h"
#include "Rtt_WinDisplayObject.h"


#pragma region Forward Declarations
namespace Interop
{
	class RuntimeEnvironment;
}
extern "C"
{
	struct lua_State;
}

#pragma endregion


namespace Rtt
{

class WinTextBoxObject : public WinDisplayObject
{
	Rtt_CLASS_NO_COPIES(WinTextBoxObject)

	public:
		typedef WinDisplayObject Super;

		#pragma region Constructors/Destructors
		WinTextBoxObject(Interop::RuntimeEnvironment& environment, const Rect& bounds);
		virtual ~WinTextBoxObject();

		#pragma endregion


		#pragma region Public Methods
		virtual Interop::UI::Control* GetControl() const;
		virtual bool Initialize();
		virtual const LuaProxyVTable& ProxyVTable() const;
		virtual int ValueForKey(lua_State *L, const char key[]) const;
		virtual bool SetValueForKey(lua_State *L, const char key[], int valueIndex);

		#pragma endregion

	private:
		#pragma region Private Methods
		/// <summary>Called when the text field control has been resized.</summary>
		/// <param name="sender">The text field control that has raised this event.</param>
		/// <param name="arguments">Empty event arguments.</param>
		void OnResized(Interop::UI::Control& sender, const Interop::EventArgs& arguments);

		/// <summary>Called when the text field control has gained the keyboard focus.</summary>
		/// <param name="sender">The text field control that has raised this event.</param>
		/// <param name="arguments">Empty event arguments.</param>
		void OnGainedFocus(Interop::UI::Control& sender, const Interop::EventArgs& arguments);

		/// <summary>Called when the text field control has lost the keyboard focus.</summary>
		/// <param name="sender">The text field control that has raised this event.</param>
		/// <param name="arguments">Empty event arguments.</param>
		void OnLostFocus(Interop::UI::Control& sender, const Interop::EventArgs& arguments);

		/// <summary>
		///  <para>Called when the user has changed the text within the text field.</para>
		///  <para>Does not get called if the text was changed programmatically via the SetText() method.</para>
		/// </summary>
		/// <param name="sender">The text field control that has raised this event.</param>
		/// <param name="arguments">Provides information about the text changes.</param>
		void OnTextChanged(Interop::UI::TextBox& sender, const Interop::UI::UITextChangedEventArgs& arguments);

		/// <summary>
		///  <para>Called when the enter key has been pressed from a text field.</para>
		/// </summary>
		/// <param name="sender">The text field control that has raised this event.</param>
		/// <param name="arguments">
		///  <para>Call this object's SetHandled() method to prevent the text field from processing the enter key.</para>
		///  <para>
		///   This will prevent it from making an error sound since it's an invalid key for single line text fields.
		///  </para>
		/// </param>
		void OnPressedEnterKey(Interop::UI::TextBox& sender, Interop::HandledEventArgs& arguments);

		#pragma endregion


		#pragma region Private Static Functions
		/// <summary>Called by the Lua setReturnKey() function.</summary>
		/// <param name="L">Pointer to the Lua state that invoked this function.</param>
		static int OnSetReturnKey(lua_State *L);

		/// <summary>Called by the Lua setTextColor() function.</summary>
		/// <param name="L">Pointer to the Lua state that invoked this function.</param>
		static int OnSetTextColor(lua_State *L);

		/// <summary>Called by the Lua onSetSelection() function.</summary>
		/// <param name="L">Pointer to the Lua state that invoked this function.</param>
		static int OnSetSelection(lua_State *L);

		#pragma endregion


		#pragma region Private Member Variables
		/// <summary>Pointer to the Win32 text box control this Corona display object wraps.</summary>
		Interop::UI::TextBox* fTextBoxPointer;

		/// <summary>
		///  <para>Set true if the placeholder text was set to nil in Lua, the default value.</para>
		///  <para>
		///   This is needed to duplicate the behavior on Android and iOS since placeholder text can never be
		///   set to null for native Win32 text fields.
		///  </para>
		/// </summary>
		bool fIsPlaceholderTextNil;

		/// <summary>Handler to be invoked when the "Resized" event has been raised.</summary>
		Interop::UI::TextBox::ResizedEvent::MethodHandler<WinTextBoxObject> fResizedEventHandler;

		/// <summary>Handler to be invoked when the "GainedFocus" event has been raised.</summary>
		Interop::UI::TextBox::GainedFocusEvent::MethodHandler<WinTextBoxObject> fGainedFocusEventHandler;

		/// <summary>Handler to be invoked when the "LostFocus" event has been raised.</summary>
		Interop::UI::TextBox::LostFocusEvent::MethodHandler<WinTextBoxObject> fLostFocusEventHandler;

		/// <summary>Handler to be invoked when the "TextChanged" event has been raised.</summary>
		Interop::UI::TextBox::TextChangedEvent::MethodHandler<WinTextBoxObject> fTextChangedEventHandler;

		/// <summary>Handler to be invoked when the "PressedEnterKey" event has been raised.</summary>
		Interop::UI::TextBox::PressedEnterKeyEvent::MethodHandler<WinTextBoxObject> fPressedEnterKeyEventHandler;

		#pragma endregion
};

}	// namespace Rtt
