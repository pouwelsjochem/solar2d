//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Config.h"

#include "Renderer/Rtt_GLTexture.h"

#include "Renderer/Rtt_GL.h"
#include "Renderer/Rtt_Texture.h"
#include "Core/Rtt_Assert.h"

#include "Rtt_Profiling.h"

// ----------------------------------------------------------------------------

#define ENABLE_DEBUG_PRINT    0

#if ENABLE_DEBUG_PRINT
    #define DEBUG_PRINT( ... ) Rtt_LogException( __VA_ARGS__ );
#else
    #define DEBUG_PRINT( ... )
#endif


#if !defined(Rtt_OPENGLES) && !defined(GL_ABGR_EXT)
#define GL_ABGR_EXT 0x8000
#endif
// ----------------------------------------------------------------------------

namespace /*anonymous*/
{
    using namespace Rtt;

    void getFormatTokens( Texture::Format format, GLint& internalFormat, GLenum& sourceFormat, GLenum& sourceType )
    {
        switch( format )
        {
            case Texture::kAlpha:        internalFormat = GL_ALPHA;        sourceFormat = GL_ALPHA;        sourceType = GL_UNSIGNED_BYTE; break;
#if defined( Rtt_MetalANGLE)
            case Texture::kLuminance:   internalFormat = GL_RED_EXT;    sourceFormat = GL_RED_EXT;        sourceType = GL_UNSIGNED_BYTE; break;
#else
            case Texture::kLuminance:    internalFormat = GL_LUMINANCE;    sourceFormat = GL_LUMINANCE;    sourceType = GL_UNSIGNED_BYTE; break;
#endif
            case Texture::kRGB:            internalFormat = GL_RGB;        sourceFormat = GL_RGB;            sourceType = GL_UNSIGNED_BYTE; break;
            case Texture::kRGBA:        internalFormat = GL_RGBA;        sourceFormat = GL_RGBA;            sourceType = GL_UNSIGNED_BYTE; break;
#if !defined( Rtt_OPENGLES )
            case Texture::kARGB:        internalFormat = GL_RGBA8;        sourceFormat = GL_BGRA;            sourceType = GL_UNSIGNED_INT_8_8_8_8_REV; break;
            case Texture::kBGRA:        internalFormat = GL_RGBA8;        sourceFormat = GL_BGRA;            sourceType = GL_UNSIGNED_INT_8_8_8_8; break;
            #ifdef GL_ABGR_EXT
            case Texture::kABGR:
                internalFormat = GL_ABGR_EXT;
                sourceFormat = GL_ABGR_EXT;
                sourceType = GL_UNSIGNED_BYTE;
                break;
            #endif
#else
            // NOTE: These are not available on OpenGL-ES
            // case Texture::kARGB:        internalFormat = GL_RGBA;        sourceFormat = GL_RGBA;            sourceType = GL_UNSIGNED_BYTE; break;
            // case Texture::kBGRA:        internalFormat = GL_RGBA;        sourceFormat = GL_RGBA;            sourceType = GL_UNSIGNED_BYTE; break;
            // case Texture::kABGR:        internalFormat = GL_RGBA;        sourceFormat = GL_RGBA;            sourceType = GL_UNSIGNED_BYTE; break;
#endif
#ifdef Rtt_NXS_ENV
            case Texture::kLuminanceAlpha:        internalFormat = GL_LUMINANCE_ALPHA;    sourceFormat = GL_LUMINANCE_ALPHA;        sourceType = GL_UNSIGNED_BYTE; break;
#endif
            default: Rtt_ASSERT_NOT_REACHED();
        }
    }

    void getFilterTokens( Texture::Filter filter, GLenum& minFilter, GLenum& magFilter )
    {
        switch( filter )
        {
            case Texture::kNearest:    minFilter = GL_NEAREST;    magFilter = GL_NEAREST;    break;
            case Texture::kLinear:    minFilter = GL_LINEAR;    magFilter = GL_LINEAR;    break;
            default: Rtt_ASSERT_NOT_REACHED();
        }
    }

    GLenum convertWrapToken( Texture::Wrap wrap )
    {
        GLenum result = GL_CLAMP_TO_EDGE;

        switch( wrap )
        {
            case Texture::kClampToEdge:        result = GL_CLAMP_TO_EDGE; break;
            case Texture::kRepeat:            result = GL_REPEAT; break;
            case Texture::kMirroredRepeat:    result = GL_MIRRORED_REPEAT; break;
            default: Rtt_ASSERT_NOT_REACHED();
        }

        return result;
    }
}

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

GLint CalculateOptimalAlignment(U32 width, GLenum format)
{
    int bytesPerPixel = 4;
    
    switch(format) {
        case GL_ALPHA:
#if defined( Rtt_MetalANGLE)
        case GL_RED_EXT:
#else
        case GL_LUMINANCE:
#endif
            bytesPerPixel = 1;
            break;
            
        case GL_RGB:
            bytesPerPixel = 3;
            break;
            
        
        #if defined(GL_ABGR_EXT)
        case GL_ABGR_EXT:
        #endif
        case GL_RGBA:
            bytesPerPixel = 4;
            break;
            
#ifdef Rtt_NXS_ENV
        case GL_LUMINANCE_ALPHA:
            bytesPerPixel = 2;
            break;
#endif
        default: // Default
            bytesPerPixel = 4;
            break;
    }

    const U32 rowBytes = width * bytesPerPixel;
   
    if(rowBytes % 8 == 0) return 8;
    if(rowBytes % 4 == 0) return 4;
    if(rowBytes % 2 == 0) return 2;
    
    return 1; // No alignment
}

void
GLTexture::Create( CPUResource* resource )
{
	Rtt_ASSERT( CPUResource::kTexture == resource->GetType() );
    Texture* texture = static_cast<Texture*>( resource );

    SUMMED_TIMING( gltc, "Texture GPU Resource: Create" );

    GLuint name = 0;
    glGenTextures( 1, &name );
    fHandle = NameToHandle( name );
    GL_CHECK_ERROR();

    GLenum minFilter;
    GLenum magFilter;
    getFilterTokens( texture->GetFilter(), minFilter, magFilter );

    glBindTexture( GL_TEXTURE_2D, name );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter );
    GL_CHECK_ERROR();

    GLenum wrapS = convertWrapToken( texture->GetWrapX() );
    GLenum wrapT = convertWrapToken( texture->GetWrapY() );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT );
    GL_CHECK_ERROR();

    GLint internalFormat;
    GLenum format;
    GLenum type;
    Texture::Format textureFormat = texture->GetFormat();
    getFormatTokens( textureFormat, internalFormat, format, type );

    const U32 w = texture->GetWidth();
    const U32 h = texture->GetHeight();
    const U8* data = texture->GetData();
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, CalculateOptimalAlignment(w, internalFormat));
        GL_CHECK_ERROR();

        // It is valid to pass a NULL pointer, so allocation is done either way
        glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, type, data );
        GL_CHECK_ERROR();
        
        fCachedFormat = internalFormat;
        fCachedWidth = w;
        fCachedHeight = h;
    }
    texture->ReleaseData();

    DEBUG_PRINT( "%s : OpenGL name: %d\n",
                    Rtt_FUNCTION,
                    name );
}

void
GLTexture::Update( CPUResource* resource )
{
    Rtt_ASSERT( CPUResource::kTexture == resource->GetType() );
    Texture* texture = static_cast<Texture*>( resource );

    SUMMED_TIMING( gltu, "Texture GPU Resource: Update" );

    const U8* data = texture->GetData();
    if( data )
    {
        const U32 w = texture->GetWidth();
        const U32 h = texture->GetHeight();
        GLint internalFormat;
        GLenum format;
        GLenum type;
        getFormatTokens( texture->GetFormat(), internalFormat, format, type );

        glBindTexture( GL_TEXTURE_2D, GetName() );

        glPixelStorei(GL_UNPACK_ALIGNMENT, CalculateOptimalAlignment(w, internalFormat));
        GL_CHECK_ERROR();

        if (internalFormat == fCachedFormat && w == fCachedWidth && h == fCachedHeight )
        {
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, format, type, data );
        }
        else
        {
            glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, type, data );
            fCachedFormat = internalFormat;
            fCachedWidth = w;
            fCachedHeight = h;
        }
        GL_CHECK_ERROR();
    }
    texture->ReleaseData();
}

void
GLTexture::Destroy()
{
    GLuint name = GetName();
    if ( 0 != name )
    {
        glDeleteTextures( 1, &name );
        GL_CHECK_ERROR();
        fHandle = 0;
    }

    DEBUG_PRINT( "%s : OpenGL name: %d\n",
                    Rtt_FUNCTION,
                    name );
}

void
GLTexture::Bind( U32 unit )
{
    glActiveTexture( GL_TEXTURE0 + unit );
    glBindTexture( GL_TEXTURE_2D, GetName() );
    GL_CHECK_ERROR();
}

GLuint
GLTexture::GetName()
{
    return HandleToName( fHandle );
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------
