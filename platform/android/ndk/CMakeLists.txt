cmake_minimum_required(VERSION 3.4.1)

set(COMMON_COMPILER_FLAGS "-Wno-deprecated -O3 -fstrict-aliasing -fvisibility=hidden -ffunction-sections -fdata-sections")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILER_FLAGS} -std=c++14")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COMMON_COMPILER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=16384")

add_definitions(
	-DALMIXER_COMPILE_WITHOUT_SDL
	-DAVOID_TABLES
	-DDISABLE_IMPORTGL
	-DLUA_USE_DLOPEN
	-DLUA_USE_POSIX
)

include_directories (
	jni
	${CORONA_ROOT}
	${CORONA_ROOT}/external/sqlite3
	${LuaIncludePath}
	${CORONA_ROOT}/external/rsa
	${CORONA_ROOT}/external/LuaHashMap
	${CORONA_ROOT}/external/openal-soft/include/AL
	${CORONA_ROOT}/external/ALmixer
	${CORONA_ROOT}/external/ALmixer/Isolated
	${CORONA_ROOT}/external/lpng1256
	${CORONA_ROOT}/external/luasocket/src
	${CORONA_ROOT}/external/lpeg
	${CORONA_ROOT}/external/luafilesystem/src
	${CORONA_ROOT}/librtt
	${CORONA_ROOT}/librtt/Corona
	${CORONA_ROOT}/plugins/shared
	${CORONA_ROOT}/platform/android/ndk
	${CORONA_ROOT}/platform/shared
    ${ANDROID_NDK_ROOT}/sources/android/cpufeatures
)

SET( LUA_SOURCES
	"${CORONA_ROOT}/platform/android/ndk/shell.lua"
	"${CORONA_ROOT}/platform/resources/init.lua"
	"${CORONA_ROOT}/platform/resources/json.lua"
	"${CORONA_ROOT}/platform/resources/dkjson.lua"
	"${CORONA_ROOT}/subrepos/easing/easing.lua"
	"${CORONA_ROOT}/librtt/Corona/CoronaLibrary.lua"
	"${CORONA_ROOT}/librtt/Corona/CoronaPrototype.lua"
	"${CORONA_ROOT}/librtt/Corona/CoronaProvider.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_add_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_average_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_color_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_colorBurn_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_colorDodge_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_darken_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_difference_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_exclusion_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_glow_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_hardLight_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_hardMix_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_hue_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_lighten_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_linearLight_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_luminosity_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_multiply_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_negation_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_normalMapWith1DirLight_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_normalMapWith1PointLight_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_overlay_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_phoenix_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_pinLight_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_reflect_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_saturation_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_screen_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_softLight_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_subtract_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_vividLight_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_composite_yuv420f_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_default_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_bloom_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_blur_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_blurGaussian_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_blurHorizontal_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_blurVertical_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_blurGaussianLinear_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_blurLinearHorizontal_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_blurLinearVertical_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_brightness_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_bulge_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_chromaKey_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_color_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_colorChannelOffset_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_colorMatrix_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_colorPolynomial_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_contrast_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_crosshatch_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_crystallize_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_desaturate_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_dissolve_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_duotone_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_emboss_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_exposure_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_frostedGlass_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_grayscale_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_hue_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_invert_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_iris_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_levels_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_linearWipe_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_median_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_monotone_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_opTile_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_pixelate_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_polkaDots_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_posterize_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_radialWipe_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_saturate_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_scatter_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_sepia_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_sharpenLuminance_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_sobel_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_step_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_straighten_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_swirl_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_vignette_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_vignetteMask_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_water_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_wobble_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_woodCut_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_filter_zoomBlur_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_checkerboard_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_lenticularHalo_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_linearGradient_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_marchingAnts_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_perlinNoise_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_radialGradient_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_random_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_stripes_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/kernel_generator_sunbeams_gl.lua"
	"${CORONA_ROOT}/librtt/Display/Shader/shell_default_gl.lua"
	"${CORONA_ROOT}/external/lpeg/re.lua"
)

SET( LUA_SOCKET_SOURCES
	"${CORONA_ROOT}/external/luasocket/src/ftp.lua"
	"${CORONA_ROOT}/external/luasocket/src/headers.lua"
	"${CORONA_ROOT}/external/luasocket/src/http.lua"
	"${CORONA_ROOT}/external/luasocket/src/ltn12.lua"
	"${CORONA_ROOT}/external/luasocket/src/mbox.lua"
	"${CORONA_ROOT}/external/luasocket/src/mime.lua"
	"${CORONA_ROOT}/external/luasocket/src/smtp.lua"
	"${CORONA_ROOT}/external/luasocket/src/socket.lua"
	"${CORONA_ROOT}/external/luasocket/src/ssl.lua"
	"${CORONA_ROOT}/external/luasocket/src/tp.lua"
	"${CORONA_ROOT}/external/luasocket/src/url.lua"
)


FOREACH(LUA_FILE ${LUA_SOURCES})
	lua_2_cpp("${LUA_FILE}")
ENDFOREACH()

FOREACH(LUA_FILE ${LUA_SOCKET_SOURCES})
	lua_2_cpp("${LUA_FILE}" luasocket)
ENDFOREACH()



add_library( corona SHARED
		jni/importgl.c
		jni/JavaToNativeBridge.cpp
		jni/JavaToNativeShim.cpp
		jni/NativeToJavaBridge.cpp

		Rtt_LuaLibOpenSLES.cpp
        Rtt_PlatformOpenSLESPlayer.cpp
		Rtt_AndroidBitmap.cpp
		Rtt_AndroidCrypto.cpp
		Rtt_AndroidData.cpp
		Rtt_AndroidDisplayObject.cpp
		Rtt_AndroidInputDevice.cpp
		Rtt_AndroidInputDeviceManager.cpp
		Rtt_AndroidDevice.cpp
		Rtt_AndroidPlatform.cpp
		Rtt_AndroidRuntimeDelegate.cpp
		Rtt_AndroidScreenSurface.cpp
		Rtt_AndroidSystemOpenEvent.cpp
		Rtt_AndroidTimer.cpp
		Rtt_AndroidWebViewObject.cpp
		AndroidBinaryReader.cpp
		AndroidBinaryReadResult.cpp
		AndroidDisplayObjectRegistry.cpp
		AndroidFileReader.cpp
		AndroidGLView.cpp
		AndroidBaseImageDecoder.cpp
		AndroidBaseNativeImageDecoder.cpp
		AndroidImageData.cpp
		AndroidImageDecoder.cpp
		AndroidJavaImageDecoder.cpp
		AndroidKeyServices.cpp
		AndroidNativePngDecoder.cpp
		AndroidOperationResult.cpp
		AndroidZipFileEntry.cpp

		${Lua2CppOutputDir}/init.cpp
		${Lua2CppOutputDir}/easing.cpp
		${Lua2CppOutputDir}/json.cpp
		${Lua2CppOutputDir}/dkjson.cpp
		${Lua2CppOutputDir}/shell.cpp

		${CORONA_ROOT}/external/luasocket/src/auxiliar.c
		${CORONA_ROOT}/external/luasocket/src/compat.c
		${CORONA_ROOT}/external/luasocket/src/except.c
		${CORONA_ROOT}/external/luasocket/src/io.c
		${CORONA_ROOT}/external/luasocket/src/mime.c
		${CORONA_ROOT}/external/luasocket/src/select.c
		${CORONA_ROOT}/external/luasocket/src/timeout.c
		${CORONA_ROOT}/external/luasocket/src/unix.c
		${CORONA_ROOT}/external/luasocket/src/buffer.c
		${CORONA_ROOT}/external/luasocket/src/inet.c
		${CORONA_ROOT}/external/luasocket/src/luasocket.c
		${CORONA_ROOT}/external/luasocket/src/options.c
		${CORONA_ROOT}/external/luasocket/src/tcp.c
		${CORONA_ROOT}/external/luasocket/src/udp.c
		${CORONA_ROOT}/external/luasocket/src/usocket.c

		${Lua2CppOutputDir}/ftp.cpp
		${Lua2CppOutputDir}/headers.cpp
		${Lua2CppOutputDir}/http.cpp
		${Lua2CppOutputDir}/ltn12.cpp
		${Lua2CppOutputDir}/mbox.cpp
		${Lua2CppOutputDir}/mime.cpp
		${Lua2CppOutputDir}/smtp.cpp
		${Lua2CppOutputDir}/socket.cpp
		${Lua2CppOutputDir}/ssl.cpp
		${Lua2CppOutputDir}/tp.cpp
		${Lua2CppOutputDir}/url.cpp
		
		${CORONA_ROOT}/external/LuaHashMap/LuaHashMap.c
		${CORONA_ROOT}/librtt/Core/Rtt_Allocator.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_Array.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_ArrayTuple.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_Assert.c
		${CORONA_ROOT}/librtt/Core/Rtt_AutoResource.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_FileSystem.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_Fixed.c
		${CORONA_ROOT}/librtt/Core/Rtt_FixedBlockAllocator.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_FixedMath.c
		${CORONA_ROOT}/librtt/Core/Rtt_Geometry.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_Math.c
		${CORONA_ROOT}/librtt/Core/Rtt_OperationResult.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_RefCount.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_ResourceHandle.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_SharedCount.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_String.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_StringHash.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_Time.c
		${CORONA_ROOT}/librtt/Core/Rtt_UseCount.cpp
		${CORONA_ROOT}/librtt/Core/Rtt_VersionTimestamp.c
		${CORONA_ROOT}/librtt/Core/Rtt_WeakCount.cpp
		${CORONA_ROOT}/librtt/Corona/CoronaAssert.c
		${CORONA_ROOT}/librtt/Corona/CoronaEvent.cpp
		${CORONA_ROOT}/librtt/Corona/CoronaLibrary.cpp
		${CORONA_ROOT}/librtt/Corona/CoronaLog.c
		${CORONA_ROOT}/librtt/Corona/CoronaLua.cpp
		${CORONA_ROOT}/librtt/Corona/CoronaVersion.c
		${CORONA_ROOT}/librtt/Corona/CoronaGraphics.cpp
		${CORONA_ROOT}/librtt/Corona/CoronaMemory.cpp
		${CORONA_ROOT}/librtt/Corona/CoronaObjects.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_BitmapMask.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_BitmapPaint.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_BitmapPaintAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ImageSheetPaintAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_BufferBitmap.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ClosedPath.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_CompositePaint.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ContainerObject.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_CPUResourcePool.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_Display.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_DisplayDefaults.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_DisplayObject.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_GradientPaint.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_GradientPaintAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_GroupObject.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ImageFrame.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ImageSheet.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ImageSheetPaint.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ImageSheetUserdata.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_LuaLibDisplay.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_LuaLibGraphics.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ObjectHandle.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_Paint.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_PaintAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_PlatformBitmap.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_PlatformBitmapTexture.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_RectObject.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_RectPath.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_Scene.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_Shader.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderBuiltin.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderComposite.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderCompositeAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderData.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderDataAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderFactory.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderInput.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderName.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderProxy.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderResource.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShaderTypes.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShapeAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShapeAdapterCircle.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShapeAdapterRect.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShapeObject.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_ShapePath.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_SnapshotObject.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_SpriteObject.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_SpritePlayer.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_SpriteSequence.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_StageObject.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_Tesselator.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TesselatorCircle.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TesselatorRect.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TesselatorShape.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureFactory.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResource.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceBitmap.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceBitmapAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceCanvas.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceCanvasAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceCapture.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceCaptureAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceExternal.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_TextureResourceExternalAdapter.cpp
		${CORONA_ROOT}/librtt/Display/Rtt_VertexCache.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_InputAxisCollection.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_InputAxisDescriptor.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_InputAxisType.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_InputDeviceCollection.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_InputDeviceConnectionState.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_InputDeviceDescriptor.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_InputDeviceType.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_PlatformInputAxis.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_PlatformInputDevice.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_PlatformInputDeviceManager.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_ReadOnlyInputAxisCollection.cpp
		${CORONA_ROOT}/librtt/Input/Rtt_ReadOnlyInputDeviceCollection.cpp
		${CORONA_ROOT}/librtt/Rtt_Archive.cpp
		${CORONA_ROOT}/librtt/Rtt_Event.cpp
		${CORONA_ROOT}/librtt/Rtt_ExplicitTemplates.cpp
		${CORONA_ROOT}/librtt/Rtt_FilePath.cpp
		${CORONA_ROOT}/librtt/Rtt_HitTestObject.cpp
		${CORONA_ROOT}/librtt/Rtt_KeyName.cpp
		${CORONA_ROOT}/librtt/Rtt_Lua.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaAssert.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaAux.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaContainer.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaContext.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaCoronaBaseLib.c
		${CORONA_ROOT}/librtt/Rtt_LuaData.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaLibCrypto.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaLibNative.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaLibOpenAL.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaLibSQLite.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaLibSystem.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaProxy.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaProxyVTable.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaResource.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaResourceOwner.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaTableIterator.cpp
		${CORONA_ROOT}/librtt/Rtt_LuaUserdataProxy.cpp
		${CORONA_ROOT}/librtt/Rtt_Matrix.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformAudioSessionManager.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformData.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformDisplayObject.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformExitCallback.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformNotifier.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformOpenALPlayer.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformReachability.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformSurface.cpp
		${CORONA_ROOT}/librtt/Rtt_PlatformTimer.cpp
		${CORONA_ROOT}/librtt/Rtt_Preference.cpp
		${CORONA_ROOT}/librtt/Rtt_PreferenceCollection.cpp
		${CORONA_ROOT}/librtt/Rtt_PreferenceValue.cpp
		${CORONA_ROOT}/librtt/Rtt_Profiling.cpp
		${CORONA_ROOT}/librtt/Rtt_RenderingStream.cpp
		${CORONA_ROOT}/librtt/Rtt_Resource.cpp
		${CORONA_ROOT}/librtt/Rtt_Runtime.cpp
		${CORONA_ROOT}/librtt/Rtt_RuntimeDelegate.cpp
		${CORONA_ROOT}/librtt/Rtt_RuntimeDelegatePlayer.cpp
		${CORONA_ROOT}/librtt/Rtt_Scheduler.cpp
		${CORONA_ROOT}/librtt/Rtt_Transform.cpp
		${Lua2CppOutputDir}/CoronaLibrary.cpp
		${Lua2CppOutputDir}/CoronaPrototype.cpp
		${Lua2CppOutputDir}/CoronaProvider.cpp
		${Lua2CppOutputDir}/kernel_composite_add_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_average_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_color_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_colorBurn_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_colorDodge_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_darken_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_difference_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_exclusion_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_glow_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_hardLight_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_hardMix_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_hue_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_lighten_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_linearLight_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_luminosity_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_multiply_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_negation_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_normalMapWith1DirLight_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_normalMapWith1PointLight_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_overlay_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_phoenix_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_pinLight_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_reflect_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_saturation_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_screen_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_softLight_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_subtract_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_vividLight_gl.cpp
		${Lua2CppOutputDir}/kernel_composite_yuv420f_gl.cpp
		${Lua2CppOutputDir}/kernel_default_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_bloom_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_blur_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_blurGaussian_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_blurHorizontal_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_blurVertical_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_blurGaussianLinear_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_blurLinearHorizontal_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_blurLinearVertical_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_brightness_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_bulge_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_chromaKey_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_color_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_colorChannelOffset_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_colorMatrix_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_colorPolynomial_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_contrast_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_crosshatch_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_crystallize_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_desaturate_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_dissolve_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_duotone_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_emboss_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_exposure_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_frostedGlass_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_grayscale_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_hue_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_invert_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_iris_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_levels_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_linearWipe_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_median_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_monotone_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_opTile_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_pixelate_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_polkaDots_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_posterize_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_radialWipe_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_saturate_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_scatter_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_sepia_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_sharpenLuminance_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_sobel_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_step_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_straighten_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_swirl_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_vignette_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_vignetteMask_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_water_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_wobble_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_woodCut_gl.cpp
		${Lua2CppOutputDir}/kernel_filter_zoomBlur_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_checkerboard_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_lenticularHalo_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_linearGradient_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_marchingAnts_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_perlinNoise_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_radialGradient_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_random_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_stripes_gl.cpp
		${Lua2CppOutputDir}/kernel_generator_sunbeams_gl.cpp
		${Lua2CppOutputDir}/shell_default_gl.cpp

		${CORONA_ROOT}/librtt/Renderer/Rtt_CommandBuffer.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_CPUResource.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_FrameBufferObject.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_Geometry_Renderer.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GeometryPool.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GL.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GLCommandBuffer.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GLFrameBufferObject.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_FormatExtensionList.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GLGeometry.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GLProgram.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GLRenderer.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GLTexture.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_GPUResource.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_HighPrecisionTime.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_Matrix_Renderer.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_Program.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_ProgramFactory.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_RenderData.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_Renderer.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_RenderTypes.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_Texture.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_TextureBitmap.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_TextureVolatile.cpp
		${CORONA_ROOT}/librtt/Renderer/Rtt_Uniform.cpp

		${CORONA_ROOT}/plugins/shared/CoronaLuaLibrary.cpp
		${CORONA_ROOT}/plugins/shared/CoronaLuaLibraryMetadata.cpp

		${CORONA_ROOT}/external/lpng1256/png.c
		${CORONA_ROOT}/external/lpng1256/pngerror.c
		${CORONA_ROOT}/external/lpng1256/pnggccrd.c
		${CORONA_ROOT}/external/lpng1256/pngget.c
		${CORONA_ROOT}/external/lpng1256/pngmem.c
		${CORONA_ROOT}/external/lpng1256/pngpread.c
		${CORONA_ROOT}/external/lpng1256/pngread.c
		${CORONA_ROOT}/external/lpng1256/pngrio.c
		${CORONA_ROOT}/external/lpng1256/pngrtran.c
		${CORONA_ROOT}/external/lpng1256/pngrutil.c
		${CORONA_ROOT}/external/lpng1256/pngset.c
		${CORONA_ROOT}/external/lpng1256/pngtrans.c
		${CORONA_ROOT}/external/lpng1256/pngvcrd.c
		${CORONA_ROOT}/external/lpng1256/pngwio.c
		${CORONA_ROOT}/external/lpng1256/pngwrite.c
		${CORONA_ROOT}/external/lpng1256/pngwtran.c
		${CORONA_ROOT}/external/lpng1256/pngwutil.c

		${CORONA_ROOT}/external/lpeg/lpeg.c
		${Lua2CppOutputDir}/re.cpp

		${CORONA_ROOT}/external/luafilesystem/src/lfs.c

		${CORONA_ROOT}/external/sqlite3/sqlite3.c
		${CORONA_ROOT}/external/lsqlite3-7/lsqlite3.c
)

target_link_libraries ( corona lua jnlua5.1 mpg123 OpenAL almixer cpufeatures GLESv2 EGL jnigraphics dl log z OpenSLES )
