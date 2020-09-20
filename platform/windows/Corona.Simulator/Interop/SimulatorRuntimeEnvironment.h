//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core\Rtt_Macros.h"
#include "Interop\LuaMethodCallback.h"
#include "Interop\MDeviceSimulatorServices.h"
#include "Interop\RuntimeEnvironment.h"
#include "Rtt_PlatformSimulator.h"
#include "Rtt_WinInputDeviceManager.h"
#include <vector>
#include <Windows.h>

namespace Interop {

/// <summary>Provides information and services for one Corona runtime that is simulating a device.</summary>
class SimulatorRuntimeEnvironment : public RuntimeEnvironment
{
	Rtt_CLASS_NO_COPIES(SimulatorRuntimeEnvironment)

	public:
		#pragma region CreationSettings Structure
		/// <summary>Provides settings to be passed into the RuntimeEnvironment class' constructor.</summary>
		struct CreationSettings : public RuntimeEnvironment::CreationSettings
		{
			/// <summary>
			///  <para>Pointer to a device configuration to be simulated by the runtime.</para>
			///  <para>Set to null to not simulate a device. In this case, the app runs like a Win32 application.</para>
			/// </summary>
			Rtt::PlatformSimulator::Config* DeviceConfigPointer;

			/// <summary>Creates a new settings object initialized to its defaults.</summary>
			CreationSettings()
			:	RuntimeEnvironment::CreationSettings(),
				DeviceConfigPointer(nullptr)
			{
			}
		};

		#pragma endregion


		#pragma region CreationResult
		/// <summary>
		///  <para>Type returned by the SimulatorRuntimeEnvironment class' CreateUsing() static function.</para>
		///  <para>Determines if the CreateUsing() function successfully created a new runtime environment object.</para>
		///  <para>If successful, the GetPointer() method will provide the newly created runtime environment object.</para>
		/// </summary>
		typedef PointerResult<SimulatorRuntimeEnvironment> CreationResult;

		#pragma endregion


		#pragma region Public Methods
		/// <summary>
		///  Gets a pointer to an optional interface providing device simulation services for the Corona Simulator.
		/// </summary>
		/// <returns>
		///  <para>Returns a pointer to a device simulation interface if running under the Corona Simulator.</para>
		///  <para>Returns null if the Corona runtime is not simulating a device.</para>
		/// </returns>
		virtual MDeviceSimulatorServices* GetDeviceSimulatorServices() const override;

		#pragma endregion


		#pragma region Public Static Functions
		/// <summary>
		///  <para>Creates a new Corona runtime environment using the given creation/project settings.</para>
		///  <para>
		///   The Corona runtime will start running immediately upon return, provided that the
		///   "IsRuntimeCreationEnabled" setting is set to true.
		///  </para>
		///  <para>
		///   The environment's "Loaded" event will be raised before this function returns,
		///   provided that the project was successfully loaded.
		///  </para>
		///  <para>
		///   To delete the returned runtime environment object, you must pass it into this class' static Destroy() function.
		///  </para>
		/// </summary>
		/// <param name="settings">
		///  <para>Provides Corona project settings such as directory paths, the window to render to, etc.</para>
		///  <para>
		///   If the "IsRuntimeCreationEnabled" field is set false, then only an Rtt::Platform object will be
		///   created and a Corona project will never be loaded/started.
		///  </para>
		/// </param>
		/// <returns>
		///  <para>
		///   Returns a success result and a pointer to a new runtime environment object if creation was
		///   successful and was able to load the given Corona project if applicable.
		///  </para>
		///  <para>
		///   Returns a failure result if creation or project loading failed. The result's GetMessage() method
		///   will provide details as to what went wrong.
		///  </para>
		/// </returns>
		static SimulatorRuntimeEnvironment::CreationResult CreateUsing(
				const SimulatorRuntimeEnvironment::CreationSettings& settings);
		
		/// <summary>
		///  <para>
		///   Destroys the runtime environment object that was returned by this class' static CreateUsing() function.
		///  </para>
		///  <para>This will automatically terminate the runtime if it is currently running.</para>
		/// </summary>
		/// <param name="environmentPointer">
		///  <para>Pointer to the runtime enivornment to be deleted/destroyed.</para>
		///  <para>A null pointer will be safely ignored.</para>
		/// </para>
		static void Destroy(SimulatorRuntimeEnvironment* environmentPointer);

		#pragma endregion

	protected:
		#pragma region Constructors/Destructors
		/// <summary>Creates a new Corona Simulator runtime environment with the given settings.</summary>
		/// <param name="settings">
		///  <para>Provides launch settings and interop layer settings to be copied to the new environment object.</para>
		///  <para>Will throw an exception if any of the settings are invalid.</para>
		/// </param>
		SimulatorRuntimeEnvironment(const SimulatorRuntimeEnvironment::CreationSettings& settings);

		/// <summary>Terminates the runtime, if running, and deletes resources consumed by the Corona environment.</summary>
		virtual ~SimulatorRuntimeEnvironment();

		#pragma endregion


	private:
		#pragma region DeviceSimulatorServices Class
		class DeviceSimulatorServices : public MDeviceSimulatorServices
		{
			public:
				DeviceSimulatorServices(
						SimulatorRuntimeEnvironment* environmentPointer,
						const Rtt::PlatformSimulator::Config* deviceConfigPointer);

				const Rtt::PlatformSimulator::Config* GetDeviceConfig() const;
				virtual bool IsLuaExitAllowed() const override;
				virtual void* ShowNativeAlert(
								const char *title, const char *message, const char **buttonLabels,
								int buttonCount, Rtt::LuaResource* resource) override;
				virtual void CancelNativeAlert(void* alertReference) override;
				virtual void RequestRestart() override;
				virtual void RequestTerminate() override;

			private:
				SimulatorRuntimeEnvironment* fEnvironmentPointer;
				const Rtt::PlatformSimulator::Config* fDeviceConfigPointer;
		};

		#pragma endregion

		#pragma region Private Methods
		/// <summary>
		///  <para>
		///   Called when the Corona runtime finished loading the "config.lua" and just before the runtime
		///   executes the "main.lua".
		///  </para>
		///  <para>This is this object's opportunity to register Corona Simulator APIs into Lua.</para>
		/// </summary>
		/// <param name="sender">The Corona runtime environment that raised this event.</para>
		/// <param name="arguments">Empty event arguments.</param>
		void OnRuntimeLoaded(RuntimeEnvironment& sender, const EventArgs& arguments);

		/// <summary>Called when the Corona runtime is about to be terminated.</summary>
		/// <param name="sender">The Corona runtime environment that raised this event.</para>
		/// <param name="arguments">Empty event arguments.</param>
		void OnRuntimeTerminating(RuntimeEnvironment& sender, const EventArgs& arguments);

		#pragma endregion


		#pragma region Private Member Variables
		/// <summary>Handler to be invoked when the "Loaded" event has been raised.</summary>
		RuntimeEnvironment::LoadedEvent::MethodHandler<SimulatorRuntimeEnvironment> fLoadedEventHandler;

		/// <summary>Handler to be invoked when the "Terminating" event has been raised.</summary>
		RuntimeEnvironment::LoadedEvent::MethodHandler<SimulatorRuntimeEnvironment> fTerminatingEventHandler;

		/// <summary>Interface providing device simulation features for the Corona Simulator.</summary>
		DeviceSimulatorServices* fDeviceSimulatorServicesPointer;

		#pragma endregion
};

}	// namespace Interop
