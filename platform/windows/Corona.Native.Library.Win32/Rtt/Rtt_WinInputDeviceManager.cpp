//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Rtt_WinInputDeviceManager.h"
#include "Core\Rtt_Build.h"
#include "Display\Rtt_Display.h"
#include "Interop\Input\Key.h"
#include "Interop\Input\ModifierKeyStates.h"
#include "Interop\UI\Window.h"
#include "Interop\MDeviceSimulatorServices.h"
#include "Interop\RuntimeEnvironment.h"
#include "Interop\ScopedComInitializer.h"
#include "Rtt_Event.h"
#include "Rtt_MPlatformDevice.h"
#include "Rtt_Runtime.h"
#include "Rtt_WinInputDevice.h"
#include "Rtt_WinPlatform.h"
#include "Rtt_KeyName.h"
#include <WindowsX.h>

namespace
{
	const char* QwertyNameForScanCode(UINT scanCode)
	{
		switch (scanCode & 0xFFu)
		{
			case 0x10: return Rtt::KeyName::kQ;
			case 0x11: return Rtt::KeyName::kW;
			case 0x12: return Rtt::KeyName::kE;
			case 0x13: return Rtt::KeyName::kR;
			case 0x14: return Rtt::KeyName::kT;
			case 0x15: return Rtt::KeyName::kY;
			case 0x16: return Rtt::KeyName::kU;
			case 0x17: return Rtt::KeyName::kI;
			case 0x18: return Rtt::KeyName::kO;
			case 0x19: return Rtt::KeyName::kP;
			case 0x1E: return Rtt::KeyName::kA;
			case 0x1F: return Rtt::KeyName::kS;
			case 0x20: return Rtt::KeyName::kD;
			case 0x21: return Rtt::KeyName::kF;
			case 0x22: return Rtt::KeyName::kG;
			case 0x23: return Rtt::KeyName::kH;
			case 0x24: return Rtt::KeyName::kJ;
			case 0x25: return Rtt::KeyName::kK;
			case 0x26: return Rtt::KeyName::kL;
			case 0x2C: return Rtt::KeyName::kZ;
			case 0x2D: return Rtt::KeyName::kX;
			case 0x2E: return Rtt::KeyName::kC;
			case 0x2F: return Rtt::KeyName::kV;
			case 0x30: return Rtt::KeyName::kB;
			case 0x31: return Rtt::KeyName::kN;
			case 0x32: return Rtt::KeyName::kM;
			default: return NULL;
		}
	}
}

namespace Rtt
{

#pragma region Static Members
/// <summary>
///  <para>Stores a handle to the "user32.dll" library loaded via the Win32 LoadLibrary() function.</para>
///  <para>Set to null if the library has not been loaded yet.</para>
/// </summary>
static HMODULE sUser32ModuleHandle;

typedef BOOL(WINAPI *RegisterTouchWindowCallback)(HWND, ULONG);
static RegisterTouchWindowCallback sRegisterTouchWindowCallback;

typedef BOOL(WINAPI *UnregisterTouchWindowCallback)(HWND);
static UnregisterTouchWindowCallback sUnregisterTouchWindowCallback;

typedef BOOL(WINAPI *CloseTouchInputHandleCallback)(HTOUCHINPUT);
static CloseTouchInputHandleCallback sCloseTouchInputHandleCallback;

typedef BOOL(WINAPI *GetTouchInputInfoCallback)(HTOUCHINPUT, UINT, PTOUCHINPUT, int);
static GetTouchInputInfoCallback sGetTouchInputInfoCallback;

#pragma endregion


#pragma region Constructors/Destructors

WinInputDeviceManager::WinInputDeviceManager(Interop::RuntimeEnvironment& environment)
:	PlatformInputDeviceManager(&environment.GetAllocator()),
	fEnvironment(environment),
	fRuntimeLoadedEventHandler(this, &WinInputDeviceManager::OnRuntimeLoaded),
	fRuntimeResumedEventHandler(this, &WinInputDeviceManager::OnRuntimeResumed),
	fRuntimeSuspendedEventHandler(this, &WinInputDeviceManager::OnRuntimeSuspended),
	fDiscoveredDeviceEventHandler(this, &WinInputDeviceManager::OnDiscoveredDevice),
	fReceivedMessageEventHandler(this, &WinInputDeviceManager::OnReceivedMessage),
	fIsMultitouchSupported(false),
	fIsCursorVisible(true),
	fCursorStyle(WinInputDeviceManager::CursorStyle::kDefaultArrow)
{
	// Initialize member variables.
	memset(&fTouchPointStates, 0, sizeof(fTouchPointStates));
	fIsLastMouseMovePointValid = false;

	// Add event handlers.
	fEnvironment.GetLoadedEventHandlers().Add(&fRuntimeLoadedEventHandler);
	fEnvironment.GetResumedEventHandlers().Add(&fRuntimeResumedEventHandler);
	fEnvironment.GetSuspendedEventHandlers().Add(&fRuntimeSuspendedEventHandler);
	fDeviceMonitor.GetDiscoveredDeviceEventHandlers().Add(&fDiscoveredDeviceEventHandler);
	auto surfacePointer = fEnvironment.GetRenderSurface();
	if (surfacePointer)
	{
		surfacePointer->GetReceivedMessageEventHandlers().Add(&fReceivedMessageEventHandler);
	}
}

WinInputDeviceManager::~WinInputDeviceManager()
{
	// Remove event handlers.
	fEnvironment.GetLoadedEventHandlers().Remove(&fRuntimeLoadedEventHandler);
	fEnvironment.GetResumedEventHandlers().Remove(&fRuntimeResumedEventHandler);
	fEnvironment.GetSuspendedEventHandlers().Remove(&fRuntimeSuspendedEventHandler);
	fDeviceMonitor.GetDiscoveredDeviceEventHandlers().Remove(&fDiscoveredDeviceEventHandler);
	auto surfacePointer = fEnvironment.GetRenderSurface();
	if (surfacePointer)
	{
		surfacePointer->GetReceivedMessageEventHandlers().Remove(&fReceivedMessageEventHandler);
	}

	// Remove the bindings to all Windows devices.
	// We must do this before the base class' destructor destroys all PlatformInputDevice objects or else they'll
	// crash attempting to use pointers to this derived class' InputDeviceMonitor objects that they're bound to.
	const ReadOnlyInputDeviceCollection deviceCollection = this->GetDevices();
	for (int index = deviceCollection.GetCount() - 1; index >= 0; index--)
	{
		auto windowsDevicePointer = dynamic_cast<WinInputDevice*>(deviceCollection.GetByIndex(index));
		if (windowsDevicePointer)
		{
			windowsDevicePointer->Unbind();
		}
	}

	// Unregister touchscreen input support.
	if (fEnvironment.GetRenderSurface() && sUnregisterTouchWindowCallback && fIsMultitouchSupported)
	{
		sUnregisterTouchWindowCallback(fEnvironment.GetRenderSurface()->GetWindowHandle());
	}
}

#pragma endregion


#pragma region Public Methods
bool WinInputDeviceManager::IsMultitouchSupported() const
{
	return fIsMultitouchSupported;
}

void WinInputDeviceManager::SetCursorVisible(bool value)
{
	// Do not continue if this setting isn't changing.
	if (value == fIsCursorVisible)
	{
		return;
	}

	// Store the given settings.
	fIsCursorVisible = value;

	// Update the mouse cursor.
	if (fIsCursorVisible)
	{
		// Show the currently assigned mouse cursor or "wait" mouse cursor.
		auto cursorName = MAKEINTRESOURCE(fCursorStyle);
		auto cursorHandle = ::LoadCursor(nullptr, cursorName);
		if (cursorHandle)
		{
			::SetCursor(cursorHandle);
		}
	}
	else
	{
		// Hide the mouse cursor.
		::SetCursor(nullptr);
	}
}

bool WinInputDeviceManager::IsCursorVisible() const
{
	return fIsCursorVisible;
}

void WinInputDeviceManager::SetCursor(WinInputDeviceManager::CursorStyle value)
{
	// Do not continue if this setting isn't changing.
	if (value == fCursorStyle)
	{
		return;
	}

	// Store the given setting.
	fCursorStyle = value;

	// Update the mouse cursor to the given style.
	if (fIsCursorVisible)
	{
		auto cursorHandle = ::LoadCursor(nullptr, MAKEINTRESOURCE(fCursorStyle));
		if (cursorHandle)
		{
			::SetCursor(cursorHandle);
		}
	}
}

WinInputDeviceManager::CursorStyle WinInputDeviceManager::GetCursor() const
{
	return fCursorStyle;
}

#pragma endregion


#pragma region Protected Methods
PlatformInputDevice* WinInputDeviceManager::CreateUsing(const InputDeviceDescriptor& descriptor)
{
	return Rtt_NEW(GetAllocator(), WinInputDevice(fEnvironment, descriptor));
}

void WinInputDeviceManager::Destroy(PlatformInputDevice* devicePointer)
{
	Rtt_DELETE(devicePointer);
}

#pragma endregion


#pragma region Private Methods
void WinInputDeviceManager::OnRuntimeLoaded(Interop::RuntimeEnvironment& sender, const Interop::EventArgs& arguments)
{
	// First, fetch all input devices currently connected to the system before starting the input device monitor.
	// This makes each devices' info and input events available to Lua.
	// This also prevents us from dispatching Lua "inputDeviceStatus" events upon starting the device monitor.
	// Note: This collection is typically empty unless multiple Corona runtimes have existed for the lifetime
	//       of the application, such as with the Corona Simulator.
	auto deviceCollection = fDeviceMonitor.GetDeviceCollection();
	const int kDeviceCount = deviceCollection.GetCount();
	for (int index = 0; index < kDeviceCount; index++)
	{
		auto deviceInterfacePointer = deviceCollection.GetByIndex(index);
		if (deviceInterfacePointer)
		{
			auto deviceType = deviceInterfacePointer->GetDeviceInfo()->GetType();
			auto coronaDevicePointer = dynamic_cast<WinInputDevice*>(this->Add(deviceType));
			if (coronaDevicePointer)
			{
				coronaDevicePointer->BindTo(deviceInterfacePointer);
			}
		}
	}

	// Start monitoring input devices and their key/axis input events.
	fDeviceMonitor.Start();

	// Register the window for touchscreen input, if available.
	if (sender.GetRenderSurface())
	{
		// Fetch callbacks to the Win32 touch APIs. (Only available on Windows 7 and newer OS versions.)
		if (!sUser32ModuleHandle)
		{
			sUser32ModuleHandle = ::LoadLibraryW(L"User32");
			if (sUser32ModuleHandle)
			{
				sRegisterTouchWindowCallback = (RegisterTouchWindowCallback)::GetProcAddress(
						sUser32ModuleHandle, "RegisterTouchWindow");
				sUnregisterTouchWindowCallback = (UnregisterTouchWindowCallback)::GetProcAddress(
						sUser32ModuleHandle, "UnregisterTouchWindow");
				sCloseTouchInputHandleCallback = (CloseTouchInputHandleCallback)::GetProcAddress(
						sUser32ModuleHandle, "CloseTouchInputHandle");
				sGetTouchInputInfoCallback = (GetTouchInputInfoCallback)::GetProcAddress(
						sUser32ModuleHandle, "GetTouchInputInfo");
			}
		}

		// Register the window for touch input. Allows us to receive WM_TOUCH messages.
		if (sRegisterTouchWindowCallback)
		{
			auto wasRegistered = sRegisterTouchWindowCallback(sender.GetRenderSurface()->GetWindowHandle(), 0);
			if (wasRegistered)
			{
				fIsMultitouchSupported = true;
			}
		}
	}

	// Set up this app to show a virtual keyboard when a native text input control has been tapped on via a trouchscreen.
	// Notes:
	// - This is a Windows 8 exclusive feature which doesn't do this by default for Win32 apps. (Only WinRT apps.)
	// - This COM interface is not available on Windows 10, but that OS version already does this by default.
	// - We only need to enable this when the Corona runtime has a window surface to render to.
	if (sender.GetRenderSurface())
	{
		try
		{
			// Define prototype and GUIDs to Microsoft's IInputPanelConfiguration COM interface.
			// Note: These are defined in Microsoft's "inputpanelconfiguration.h" header file,
			//       but this header is not available in Visual Studio 2013. So, define them ourselves.
			class IInputPanelConfiguration : public IUnknown
			{
				public:
					virtual HRESULT STDMETHODCALLTYPE EnableFocusTracking() = 0;
			};
			struct __declspec(uuid("2853ADD3-F096-4C63-A78F-7FA3EA837FB7")) Corona_CLSID_IInputPanelConfiguration;
			struct __declspec(uuid("41C81592-514C-48BD-A22E-E6AF638521A6")) Corona_IID_IInputPanelConfiguration;

			// Attempt to aquire the Windows 8 IInputPanelConfiguration COM object and enable focus tracking.
			// Will return null on any other Windows OS version, which is okay.
			auto comApartmentType = Interop::ScopedComInitializer::ApartmentType::kSingleThreaded;
			Interop::ScopedComInitializer scopedComInitializer(comApartmentType);
			IInputPanelConfiguration* inputPanelConfigPointer = nullptr;
			::CoCreateInstance(
					_uuidof(Corona_CLSID_IInputPanelConfiguration), nullptr, CLSCTX_INPROC_SERVER,
					_uuidof(Corona_IID_IInputPanelConfiguration), (LPVOID*)&inputPanelConfigPointer);
			if (inputPanelConfigPointer)
			{
				inputPanelConfigPointer->EnableFocusTracking();
				inputPanelConfigPointer->Release();
			}
		}
		catch (...) {}
	}
}

void WinInputDeviceManager::OnRuntimeResumed(Interop::RuntimeEnvironment& sender, const Interop::EventArgs& arguments)
{
	fDeviceMonitor.Start();
}

void WinInputDeviceManager::OnRuntimeSuspended(Interop::RuntimeEnvironment& sender, const Interop::EventArgs& arguments)
{
	fDeviceMonitor.Stop();
}

void WinInputDeviceManager::OnDiscoveredDevice(
	Interop::Input::InputDeviceMonitor& sender, Interop::Input::InputDeviceInterfaceEventArgs& arguments)
{
	WinInputDevice* coronaDevicePointer;

	// Fetch a pointer to the newly discovered input device.
	auto deviceIterfacePointer = &arguments.GetDeviceInterface();

	// Do not continue if the discovered device has already been added to Corona's collection.
	const ReadOnlyInputDeviceCollection& deviceCollection = GetDevices();
	for (int index = deviceCollection.GetCount(); index >= 0; index--)
	{
		coronaDevicePointer = dynamic_cast<WinInputDevice*>(deviceCollection.GetByIndex(index));
		if (coronaDevicePointer && (coronaDevicePointer->GetDeviceInterface() == deviceIterfacePointer))
		{
			return;
		}
	}

	// Set up a Corona binding to the newly discovered input device.
	// This makes the device's info and input events available to Lua.
	auto deviceType = arguments.GetDeviceInterface().GetDeviceInfo()->GetType();
	coronaDevicePointer = dynamic_cast<WinInputDevice*>(this->Add(deviceType));
	if (!coronaDevicePointer)
	{
		return;
	}
	coronaDevicePointer->BindTo(&arguments.GetDeviceInterface());

	// Dispatch an "inputDeviceStatus" event to Lua.
	auto runtimePointer = fEnvironment.GetRuntime();
	if (runtimePointer)
	{
		bool hasConnectionStateChanged = true;
		bool wasReconfigured = false;
		Rtt::InputDeviceStatusEvent event(coronaDevicePointer, hasConnectionStateChanged, wasReconfigured);
		runtimePointer->DispatchEvent(event);
	}
}

void WinInputDeviceManager::OnReceivedMessage(
	Interop::UI::UIComponent& sender, Interop::UI::HandleMessageEventArgs& arguments)
{
	// Do not continue if the message was already handled.
	if (arguments.WasHandled())
	{
		return;
	}

	// Do not continue if Corona is not currently running.
	auto runtimePointer = fEnvironment.GetRuntime();
	if (!runtimePointer || runtimePointer->IsSuspended())
	{
		return;
	}

	// Handle the received message.
	switch (arguments.GetMessageId())
	{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
			// Dispatch a "mouse" event to Corona.
			POINT point = GetMousePointFrom(arguments.GetLParam());
			OnReceivedMouseEvent(Rtt::MouseEvent::kDown, point, 0, 0, arguments.GetWParam());

			// This function call allows us to receive the WM_LBUTTONUP message.
			::SetCapture(sender.GetWindowHandle());

			// Simulate a touch event if the message came from a mouse.
			auto touchInputStatePointer = &fTouchPointStates[0];
			if (!touchInputStatePointer->HasStarted && !WasMouseMessageGeneratedFromTouchInput())
			{
				// Store the simulated touch state.
				touchInputStatePointer->HasStarted = true;
				touchInputStatePointer->StartPoint = point;
				touchInputStatePointer->LastPoint = point;

				// Dispatch a "touch" event to Corona.
				OnReceivedTouchEvent(0, point, point, Rtt::TouchEvent::kBegan);
			}

			// Flag the message as handled.
			arguments.SetReturnResult(0);
			arguments.SetHandled();
			break;
		}
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		{
			// Dispatch a "mouse" event to Corona.
			// Note: We do not treat middle and right mouse button drags as touch events.
			POINT point = GetMousePointFrom(arguments.GetLParam());
			OnReceivedMouseEvent(Rtt::MouseEvent::kDown, point, 0, 0, arguments.GetWParam());

			// Flag the message as handled.
			arguments.SetReturnResult(0);
			arguments.SetHandled();
			break;
		}
		case WM_MOUSEMOVE:
		{
			// Fetch current mouse position.
			POINT point = GetMousePointFrom(arguments.GetLParam());

			// Do not continue if the mouse position hasn't changed. This prevents duplicate events.
			// Note: Windows sometimes sends mouse move messages when the mouse hasn't actually moved.
			//       For example, displaying the Windows "Task Manager" can trigger a mouse move message once a second.
			//       Microsoft's WM_SETREDRAW, MFC's SetRedraw(TRUE), and .NET's EndUpdate() triggers this issue.
			if (fIsLastMouseMovePointValid && (fLastMouseMovePoint.x == point.x) && (fLastMouseMovePoint.y == point.y))
			{
				break;
			}
			fLastMouseMovePoint = point;
			fIsLastMouseMovePointValid = true;

			// Dispatch a "mouse" event to Corona.
			OnReceivedMouseEvent(Rtt::MouseEvent::kMove, point, 0, 0, arguments.GetWParam());

			// Dispatch a "touch" event to Corona, but only if the left mouse button is down.
			auto touchInputStatePointer = &fTouchPointStates[0];
			if (touchInputStatePointer->HasStarted && !WasMouseMessageGeneratedFromTouchInput() &&
			    (::GetCapture() == sender.GetWindowHandle()) && (arguments.GetWParam() & MK_LBUTTON))
			{
				touchInputStatePointer->LastPoint = point;
				OnReceivedTouchEvent(0, point, touchInputStatePointer->StartPoint, Rtt::TouchEvent::kMoved);
			}

			// Flag the message as handled.
			arguments.SetReturnResult(0);
			arguments.SetHandled();
			break;
		}
		case WM_LBUTTONUP:
		{
			// Fetch current mouse position.
			POINT point = GetMousePointFrom(arguments.GetLParam());

			// Must be called after calling SetCapture() in OnLButtonDown().
			::ReleaseCapture();

			// Dispatch a "mouse" event to Corona.
			OnReceivedMouseEvent(Rtt::MouseEvent::kUp, point, 0, 0, arguments.GetWParam());

			// Only dispatch a "touch" event if:
			// - Corona has dispatched a "began" touch event phase.
			// - The touch events were not canceled, such as by showing a wait cursor.
			auto touchInputStatePointer = &fTouchPointStates[0];
			if (touchInputStatePointer->HasStarted && !WasMouseMessageGeneratedFromTouchInput())
			{
				// Clear the touch tracking flag.
				touchInputStatePointer->HasStarted = false;
				touchInputStatePointer->LastPoint = point;

				// Dispatch a "touch" event to Corona.
				OnReceivedTouchEvent(0, point, touchInputStatePointer->StartPoint, Rtt::TouchEvent::kEnded);
			}

			// Flag the message as handled.
			arguments.SetReturnResult(0);
			arguments.SetHandled();
			break;
		}
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		{
			// Dispatch a "mouse" event to Corona.
			// Note: We do not treat middle and right mouse button drags as touch events.
			POINT point = GetMousePointFrom(arguments.GetLParam());
			OnReceivedMouseEvent(Rtt::MouseEvent::kUp, point, 0, 0, arguments.GetWParam());

			// Flag the message as handled.
			arguments.SetReturnResult(0);
			arguments.SetHandled();
			break;
		}
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		{
			// Fetch current mouse position.
			POINT point;
			point.x = GET_X_LPARAM(arguments.GetLParam());
			point.y = GET_Y_LPARAM(arguments.GetLParam());
			::ScreenToClient(sender.GetWindowHandle(), &point);
			point = GetMousePointFrom(MAKELPARAM(point.x, point.y));

			// Fetch the distance traveled by the vertical or horizontal scroll wheel.
			// This is measured in Win32 "WHEEL_DELTA" units, where a value of 120 is considered 1 unit.
			float scrollWheelDeltaX = 0;
			float scrollWheelDeltaY = 0;
			if (arguments.GetMessageId() == WM_MOUSEWHEEL)
			{
				// The sign of the deltas is the opposite of what is expected so they are swapped
				scrollWheelDeltaY = -(float)GET_WHEEL_DELTA_WPARAM(arguments.GetWParam());
				if (scrollWheelDeltaY < 0)
				{
					scrollWheelDeltaY = -1;
				}
				else if (scrollWheelDeltaY > 0)
				{
					scrollWheelDeltaY = 1;
				}
			}
			else
			{
				// The sign of the deltas is the opposite of what is expected so they are swapped
				scrollWheelDeltaX = -(float)GET_WHEEL_DELTA_WPARAM(arguments.GetWParam());
				if (scrollWheelDeltaX < 0)
				{
					scrollWheelDeltaX = -1;
				}
				else if (scrollWheelDeltaX > 0)
				{
					scrollWheelDeltaX = 1;
				}
			}

			// Dispatch a "mouse" event to Corona.
			OnReceivedMouseEvent(
					Rtt::MouseEvent::kScroll, point, scrollWheelDeltaX, scrollWheelDeltaY, arguments.GetWParam());

			// Flag the message as handled.
			arguments.SetReturnResult(0);
			arguments.SetHandled();
			break;
		}
		case WM_TOUCH:
		{
			// We've received new touchscreen input information.
			auto touchInputCount = LOWORD(arguments.GetWParam());
			auto touchInputHandle = (HTOUCHINPUT)arguments.GetLParam();
			if (sGetTouchInputInfoCallback && touchInputHandle && (touchInputCount > 0))
			{
				// Fetch all of the touch input information received, up to "kMaxTouchPoints".
				// Note: Our "kMaxTouchPoints" maximum is a Win32 Corona limitation, not a Microsoft limitation.
				TOUCHINPUT touchInputs[kMaxTouchPoints];
				if (touchInputCount > kMaxTouchPoints)
				{
					touchInputCount = kMaxTouchPoints;
				}
				auto hasReceivedTouchInput = sGetTouchInputInfoCallback(
						touchInputHandle, touchInputCount, touchInputs, sizeof(TOUCHINPUT));
				if (!hasReceivedTouchInput)
				{
					touchInputCount = 0;
				}

				// Handle the received touch inputs.
				for (int touchInputIndex = 0; touchInputIndex < touchInputCount; touchInputIndex++)
				{
					// Fetch the next touch input object.
					auto touchInputPointer = &touchInputs[touchInputIndex];

					// Fetch the touch phase.
					Rtt::TouchEvent::Phase phase;
					switch (touchInputPointer->dwFlags & 0x7)
					{
						case TOUCHEVENTF_DOWN:
							phase = Rtt::TouchEvent::kBegan;
							break;
						case TOUCHEVENTF_MOVE:
							phase = Rtt::TouchEvent::kMoved;
							break;
						case TOUCHEVENTF_UP:
							phase = Rtt::TouchEvent::kEnded;
							break;
						default:
							continue;
					}

					// Fetch the touch input's current position in Corona content coordinates.
					POINT point;
					point.x = TOUCH_COORD_TO_PIXEL(touchInputPointer->x);
					point.y = TOUCH_COORD_TO_PIXEL(touchInputPointer->y);
					auto windowHandle = arguments.GetWindowHandle();
					::ScreenToClient(windowHandle, &point);

					// Fetch a touch state element in our "fTouchPointStates" array matching the received touch point.
					// We use this to track the finger's start touch, last touch, and other stateful information.
					int touchPointStateIndex = -1;
					if (touchInputPointer->dwFlags & TOUCHEVENTF_PRIMARY)
					{
						// *** This is the 1st finger to be pressed on the screen. ***
						// Always select the first element in our "fTouchPointStates" in this case.
						touchPointStateIndex = 0;
					}
					else
					{
						// *** This is the 2nd, 3rd, etc. finger on the screen. ***

						// First, check if we're already tracking the received touch point.
						int availableTouchPointStateIndex = -1;
						for (int index = 1; index < kMaxTouchPoints; index++)
						{
							if (fTouchPointStates[index].HasStarted)
							{
								if (fTouchPointStates[index].TouchInputId == touchInputPointer->dwID)
								{
									touchPointStateIndex = index;
									break;
								}
							}
							else if (availableTouchPointStateIndex < 0)
							{
								availableTouchPointStateIndex = index;
							}
						}

						// If we're not tracking the recieved touch point, then select an available slot in the array.
						if ((touchPointStateIndex < 0) && (availableTouchPointStateIndex >= 0))
						{
							touchPointStateIndex = availableTouchPointStateIndex;
						}
					}
					if (touchPointStateIndex < 0)
					{
						continue;
					}

					// Store the touch input's current state.
					auto touchPointStatePointer = &fTouchPointStates[touchPointStateIndex];
					touchPointStatePointer->TouchInputId = touchInputPointer->dwID;
					touchPointStatePointer->LastPoint = point;
					if (phase == Rtt::TouchEvent::kBegan)
					{
						if (touchPointStatePointer->HasStarted)
						{
							phase = Rtt::TouchEvent::kMoved;
						}
						else
						{
							touchPointStatePointer->HasStarted = true;
							touchPointStatePointer->StartPoint = point;
						}
					}
					else if (phase == Rtt::TouchEvent::kMoved)
					{
						if (!touchPointStatePointer->HasStarted)
						{
							continue;
						}
					}
					else if (phase == Rtt::TouchEvent::kEnded)
					{
						if (!touchPointStatePointer->HasStarted)
						{
							continue;
						}
						touchPointStatePointer->HasStarted = false;
					}

					// Dispatch a Lua "touch" event.
					OnReceivedTouchEvent(
							(uint32_t)touchPointStateIndex, touchPointStatePointer->LastPoint,
							touchPointStatePointer->StartPoint, phase);
				}
			}

			// Finalize the above touch handling.
			if (arguments.WasHandled() && touchInputHandle && sCloseTouchInputHandleCallback)
			{
				// We've successful loaded and handled the touch input above.
				// We must call the Win32 CloseTouchInputHandle() function after calling GetTouchInputInfo().
				sCloseTouchInputHandleCallback(touchInputHandle);
			}
			else
			{
				// We were not able to acquire touch input above.
				// So, we must pass the touch input to Microsoft's default handler or else an assert will occur.
				arguments.SetReturnResult(::DefWindowProc(
						arguments.GetWindowHandle(), arguments.GetMessageId(),
						arguments.GetWParam(), arguments.GetLParam()));
				arguments.SetHandled();
			}
			break;
		}
		case WM_SETCURSOR:
		{
			// Do not continue if the mouse cursor is not within the client area of the window.
			if (LOWORD(arguments.GetLParam()) != HTCLIENT)
			{
				break;
			}

			// Do not change the mouse cursor style while it's hovering over a child control.
			if (arguments.GetWindowHandle() != (HWND)arguments.GetWParam())
			{
				break;
			}

			// Update the current mouse cursor style.
			if (fIsCursorVisible)
			{
				// Show the assigned mouse cursor.
				// Optimization: If we do not handle this event, then the OS will display an arrow cursor for us.
				if ((WORD)fCursorStyle != (WORD)IDC_ARROW)
				{
					auto cursorHandle = ::LoadCursor(nullptr, MAKEINTRESOURCE(fCursorStyle));
					if (cursorHandle)
					{
						::SetCursor(cursorHandle);
						arguments.SetReturnResult(0);
						arguments.SetHandled();
					}
				}
			}
			else
			{
				// Hide the mouse cursor.
				::SetCursor(nullptr);
				arguments.SetReturnResult(0);
				arguments.SetHandled();
			}
			break;
		}
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			// Determine if the key state is down or up.
			bool isKeyDown = (arguments.GetMessageId() == WM_KEYDOWN);

			// Ignore the key event if it is being repeated.
			// Note: If bit 30 (zero based) is set to 1, then the key was previously down.
			bool wasKeyPreviouslyDown = (arguments.GetLParam() & 0x40000000) ? true : false;
			if ((isKeyDown && wasKeyPreviouslyDown) || (!isKeyDown && !wasKeyPreviouslyDown))
			{
				arguments.SetReturnResult(0);
				arguments.SetHandled();
				break;
			}

			// Fetch the key code that was pressed/released and its scan code.
			S32 keyCode = (S32)arguments.GetWParam();
			UINT scanCode = (UINT)((arguments.GetLParam() >> 16) & 0xFF);

			// If the key code for "shift", "alt", or "ctrl" has been received, then determine if
			// the left/right version of that key was pressed/released by its scan code.
			if ((VK_SHIFT == keyCode) || (VK_MENU == keyCode) || (VK_CONTROL == keyCode))
			{
				S32 result = (S32)MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
				if (result != 0)
				{
					keyCode = result;
				}
			}

			// Fetch the current state of the "shift", "alt", and "ctrl" keys.
			auto modifierKeyStates = Interop::Input::ModifierKeyStates::FromKeyboard();

			// Dispatch a "key" event to Lua.
			auto keyInfo = Interop::Input::Key::FromNativeCode(keyCode);
			const char* qwertyName = QwertyNameForScanCode(scanCode);
			Rtt::KeyEvent keyEvent(
					nullptr, isKeyDown ? Rtt::KeyEvent::kDown : Rtt::KeyEvent::kUp,
					keyInfo.GetCoronaName(), keyCode,
					modifierKeyStates.IsShiftDown(), modifierKeyStates.IsAltDown(),
					modifierKeyStates.IsControlDown(), modifierKeyStates.IsCommandDown(),
					qwertyName);
			fEnvironment.GetRuntime()->DispatchEvent(keyEvent);
			if (keyEvent.GetResult())
			{
				// The Lua key listener returned true. Flag the Windows message as handled.
				arguments.SetReturnResult(0);
				arguments.SetHandled();
			}
			break;
		}
		case WM_CHAR:
		{
			wchar_t wParam = (wchar_t)arguments.GetWParam();
			wchar_t wParamArray[2] = { wParam, 0 };

			WinString stringConverter;
			stringConverter.SetUTF16(wParamArray);
			int utf8Length = strlen(stringConverter.GetUTF8()) + 1;
			char * utf8Character = new char[utf8Length];
			strcpy_s(utf8Character, utf8Length, stringConverter.GetUTF8());
			if (strlen(utf8Character) > 1 || isprint(utf8Character[0]))
			{
				Rtt::CharacterEvent characterEvent(nullptr, utf8Character);
				runtimePointer->DispatchEvent(characterEvent);
			}
			break;
		}
		case WM_APPCOMMAND:
		{
//TODO: WM_APPCOMMAND messages need to be handled by the main window too because these messages
//      will not be received here if the mouse is hovering outside of the render surface control.

			// We've received an "Application Command" message from a media keyboard or mouse.
			auto appCommandId = GET_APPCOMMAND_LPARAM(arguments.GetLParam());
			switch (appCommandId)
			{
				case APPCOMMAND_BROWSER_BACKWARD:
				case APPCOMMAND_BROWSER_FORWARD:
				{
					// A browse back/forward command has been received.
					// Simulate its equivalent Windows key messages to be delivered to Corona as Lua key events.
					Interop::UI::MessageSettings messageSettings;
					messageSettings.WindowHandle = arguments.GetWindowHandle();
					if (APPCOMMAND_BROWSER_BACKWARD == appCommandId)
					{
						messageSettings.WParam = VK_BROWSER_BACK;
					}
					else
					{
						messageSettings.WParam = VK_BROWSER_FORWARD;
					}

					// Simulate a Windows key down message.
					messageSettings.MessageId = WM_KEYDOWN;
					messageSettings.LParam = 0;
					Interop::UI::HandleMessageEventArgs keyDownMessageEventArgs(messageSettings);
					this->OnReceivedMessage(sender, keyDownMessageEventArgs);

					// Simulate a Windows key up message.
					messageSettings.MessageId = WM_KEYUP;
					messageSettings.LParam = 0x40000000L;
					Interop::UI::HandleMessageEventArgs keyUpMessageEventArgs(messageSettings);
					this->OnReceivedMessage(sender, keyUpMessageEventArgs);

					// If the above key messages were handled, then set the return result to true.
					// Returning true prevents this application command from be passed to the parent window or desktop.
					if (keyDownMessageEventArgs.WasHandled() || keyUpMessageEventArgs.WasHandled())
					{
						arguments.SetReturnResult((LRESULT)TRUE);
						arguments.SetHandled();
					}
					break;
				}
			}
			break;
		}
	}
}

void WinInputDeviceManager::OnReceivedMouseEvent(
	Rtt::MouseEvent::MouseEventType eventType, POINT& point,
	float scrollWheelDeltaX, float scrollWheelDeltaY, WPARAM mouseButtonFlags)
{
	// Fetch the Corona runtime.
	auto runtimePointer = fEnvironment.GetRuntime();
	if (!runtimePointer)
	{
		return;
	}

	// Determine which mouse buttons are primary and secondary.
	// Typically, the left mouse button is the primary, unless it has been swapped in the Control Panel.
	bool areMouseButtonsNotSwapped = ::GetSystemMetrics(SM_SWAPBUTTON) ? false : true;
	UINT primaryButtonMask = areMouseButtonsNotSwapped ? MK_LBUTTON : MK_RBUTTON;
	UINT secondaryButtonMask = areMouseButtonsNotSwapped ? MK_RBUTTON : MK_LBUTTON;

	// Fetch the mouse's current up/down buttons states.
	bool isPrimaryDown = (mouseButtonFlags & primaryButtonMask) ? true : false;
	bool isSecondaryDown = (mouseButtonFlags & secondaryButtonMask) ? true : false;
	bool isMiddleDown = (mouseButtonFlags & MK_MBUTTON) ? true : false;

	// Determine if this is a "drag" event.
	if ((Rtt::MouseEvent::kMove == eventType) && (isPrimaryDown || isSecondaryDown || isMiddleDown))
	{
		eventType = Rtt::MouseEvent::kDrag;
	}

	// Fetch the current state of the "shift", "alt", and "ctrl" keys.
	auto modifierKeyStates = Interop::Input::ModifierKeyStates::FromKeyboard();

	// Dispatch the mouse event to Lua.
	Rtt::MouseEvent mouseEvent(
			eventType,
			Rtt_IntToReal(point.x), Rtt_IntToReal(point.y),
			Rtt_FloatToReal(scrollWheelDeltaX), Rtt_FloatToReal(scrollWheelDeltaY), 0,
			isPrimaryDown, isSecondaryDown, isMiddleDown,
			modifierKeyStates.IsShiftDown(), modifierKeyStates.IsAltDown(),
			modifierKeyStates.IsControlDown(), modifierKeyStates.IsCommandDown());
	runtimePointer->DispatchEvent(mouseEvent);
}

void WinInputDeviceManager::OnReceivedTouchEvent(
	uint32_t touchIndex, POINT currentPosition, POINT startPosition, Rtt::TouchEvent::Phase phase)
{
	// Fetch the Corona runtime.
	auto runtimePointer = fEnvironment.GetRuntime();
	if (!runtimePointer)
	{
		return;
	}

	// Dispatch a "touch" event to Lua.
	Rtt::TouchEvent event(
			Rtt_IntToReal(currentPosition.x),
			Rtt_IntToReal(currentPosition.y),
			Rtt_IntToReal(startPosition.x),
			Rtt_IntToReal(startPosition.y),
			phase);
	event.SetId((const void*)(touchIndex + 1));
	if (fEnvironment.GetPlatform()->GetDevice().DoesNotify(Rtt::MPlatformDevice::kMultitouchEvent))
	{
		runtimePointer->DispatchEvent(Rtt::MultitouchEvent(&event, 1));
	}
	else if (0 == touchIndex)
	{
		runtimePointer->DispatchEvent(event);
	}
}

POINT WinInputDeviceManager::GetMousePointFrom(LPARAM LParam)
{
	// Fetch the mouse coordinate from the Windows message's LPARAM.
	POINT point;
	point.x = GET_X_LPARAM(LParam);
	point.y = GET_Y_LPARAM(LParam);
	return point;
}

bool WinInputDeviceManager::WasMouseMessageGeneratedFromTouchInput()
{
	// Microsoft documents that this is how you detect if a Win32 mouse message was generated by a touch event.
	// Unfortunately, Microsoft does not provide any constants for this.
	return ((::GetMessageExtraInfo() & 0xFFFFFF00) == 0xFF515700);
}

#pragma endregion

}	// namespace Rtt
