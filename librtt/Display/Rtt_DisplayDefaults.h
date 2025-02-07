//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_DisplayDefaults_H__
#define _Rtt_DisplayDefaults_H__

#include "Core/Rtt_Types.h"
#include "Display/Rtt_Paint.h"
#include "Renderer/Rtt_RenderTypes.h"

// ----------------------------------------------------------------------------

namespace Rtt
{
    
struct TimeTransform;

// ----------------------------------------------------------------------------

class DisplayDefaults
{
    public:
        DisplayDefaults();

	public:
		Color GetClearColor() const { return fClearColor; }
		Color GetFillColor() const { return fFillColor; }
		float GetAnchorX() const { return fAnchorX; }
		float GetAnchorY() const { return fAnchorY; }
		bool IsAnchorClamped() const { return fIsAnchorClamped; }
		
		void SetClearColor( Color newValue ) { fClearColor = newValue; }
		void SetFillColor( Color newValue ) { fFillColor = newValue; }
		void SetAnchorX( float newValue ) { fAnchorX = newValue; }
		void SetAnchorY( float newValue ) { fAnchorY = newValue; }
		void SetAnchorClamped( bool newValue ) { fIsAnchorClamped = newValue; }
		
	public:
		RenderTypes::TextureFilter GetMagTextureFilter() const { return (RenderTypes::TextureFilter)fMagTextureFilter; }
		void SetMagTextureFilter( RenderTypes::TextureFilter newValue ) { fMagTextureFilter = newValue; }

        RenderTypes::TextureFilter GetMinTextureFilter() const { return (RenderTypes::TextureFilter)fMinTextureFilter; }
        void SetMinTextureFilter( RenderTypes::TextureFilter newValue ) { fMinTextureFilter = newValue; }

        RenderTypes::TextureWrap GetTextureWrapX() const { return (RenderTypes::TextureWrap)fWrapX; }
        void SetTextureWrapX( RenderTypes::TextureWrap newValue ) { fWrapX = newValue; }

        RenderTypes::TextureWrap GetTextureWrapY() const { return (RenderTypes::TextureWrap)fWrapY; }
        void SetTextureWrapY( RenderTypes::TextureWrap newValue ) { fWrapY = newValue; }

		bool IsImageSheetSampledInsideFrame() const { return fIsImageSheetSampledInsideFrame;}
		void SetImageSheetSampledInsideFrame( bool newValue ) { fIsImageSheetSampledInsideFrame = newValue; }

		bool IsExternalTextureRetina() const { return fIsExternalTextureRetina;}
		void SetExternalTextureRetina( bool newValue ) { fIsExternalTextureRetina = newValue; }

	public:
		bool IsShaderCompilerVerbose() const { return fShaderCompilerVerbose; }
		void SetShaderCompilerVerbose( bool newValue ) { fShaderCompilerVerbose = newValue; }

	private:
		Color fClearColor;
		Color fFillColor;
		float fAnchorX;
		float fAnchorY;
		U8 fMagTextureFilter;
		U8 fMinTextureFilter;
		U8 fWrapX;
		U8 fWrapY;
		bool fShaderCompilerVerbose;
		bool fIsAnchorClamped;
		bool fIsImageSheetSampledInsideFrame;
		bool fIsExternalTextureRetina;
        bool fSkipsCull;
        bool fSkipsHitTest;
        bool fEnableDepthInScene;
        bool fEnableStencilInScene;
        bool fAddDepthToResource;
        bool fAddStencilToResource;
        float fSceneDepthClear;
        float fAddedDepthClear;
        U32 fSceneStencilClear;
        U32 fAddedStencilClear;
        TimeTransform *fTimeTransform;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_DisplayDefaults_H__
