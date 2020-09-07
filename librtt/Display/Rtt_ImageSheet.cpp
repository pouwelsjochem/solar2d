//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Display/Rtt_ImageSheet.h"

#include "Core/Rtt_Build.h"
#include "Core/Rtt_Math.h"
#include "Display/Rtt_BitmapPaint.h"
#include "Display/Rtt_Display.h"
#include "Display/Rtt_DisplayDefaults.h"
#include "Display/Rtt_ImageFrame.h"
#include "Display/Rtt_ImageSheetUserdata.h"
#include "Display/Rtt_TextureFactory.h"
#include "Display/Rtt_TextureResource.h"
#include "Renderer/Rtt_Texture.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaContext.h"
#include "Rtt_LuaLibSystem.h"
#include "Rtt_MPlatform.h"
#include "Rtt_Runtime.h"

// ----------------------------------------------------------------------------
namespace Rtt {
// ----------------------------------------------------------------------------

const char ImageSheet::kMetatableName[] = "ImageSheet";  // unique identifier for this userdata type
int ImageSheet::CreateAndPush(lua_State *L, Rtt_Allocator *allocator) {
    int nextArg = 1; // Required 1st param is "filename" filename [, baseDirectory]
    MPlatform::Directory baseDir = MPlatform::kResourceDir;
    const char *imageName = LuaLibSystem::GetFilename(L, nextArg, baseDir);
    if (imageName) {
        if (lua_istable(L, nextArg)) {
            Runtime *runtime = LuaContext::GetRuntime(L);

            // Image sheets should be loaded at full resolution
            TextureFactory &factory = runtime->GetDisplay().GetTextureFactory();
            SharedPtr<TextureResource> texture = factory.FindOrCreate(imageName, baseDir, PlatformBitmap::kIsBitsFullResolution, false);
            if (Rtt_VERIFY(texture.NotNull())) {
                ImageSheet *sheet = Rtt_NEW(allocator, ImageSheet(allocator, texture));
                sheet->Initialize(L, nextArg);

                AutoPtr<ImageSheet> pSheet(allocator, sheet);
                ImageSheetUserdata *sheetUserData = Rtt_NEW(allocator, ImageSheetUserdata(pSheet));
                if (sheet->GetNumFrames() <= 0) {
					luaL_argerror(L, nextArg, "no frames defined");
                }

                if (Rtt_VERIFY(sheetUserData)) {
                    Lua::PushUserdata(L, sheetUserData, Self::kMetatableName);
					return 1;
                }
            }
        } else {
            luaL_argerror(L, nextArg, "table (options) expected");
        }
    } else {
        luaL_argerror(L, nextArg, "string (filename) expected");
    }
	return 0;
}

ImageSheetUserdata *
ImageSheet::ToUserdata(lua_State *L, int index) {
    return (ImageSheetUserdata *)Lua::CheckUserdata(L, index, Self::kMetatableName);
}

int ImageSheet::Finalizer(lua_State *L) {
    ImageSheetUserdata **sheetUserData = (ImageSheetUserdata **)luaL_checkudata(L, 1, Self::kMetatableName);
    if (sheetUserData) {
        Rtt_DELETE(*sheetUserData);
    }
    return 0;
}

// Call this to init metatable
void ImageSheet::Initialize(lua_State *L) {
    Rtt_LUA_STACK_GUARD(L);
	
    const luaL_Reg kVTable[] = {{"__gc", Self::Finalizer}, {NULL, NULL}};
    Lua::InitializeMetatable(L, Self::kMetatableName, kVTable);
}

ImageSheet::ImageSheet(Rtt_Allocator *allocator, const SharedPtr<TextureResource> &resource) : fResource(resource), fFrames(allocator) {
	
}

ImageSheet::~ImageSheet() {
}

/*

Processing the 'options' table
===============================================================================
local options = {numFrames = 3, height = 50, width = 50}
local options = {frames = {{x = 420, y = 420, width = 50, height = 50}, {x = 69, y = 42, width = 10, height = 10}}}
*/
int ImageSheet::Initialize(lua_State *L, int optionsIndex) {
#ifdef Rtt_DEBUG
    int top = lua_gettop(L);
#endif
    Rtt_ASSERT(lua_istable(L, optionsIndex));
    Rtt_ASSERT(optionsIndex > 0);  // need stable index

    Rtt_Allocator *allocator = LuaContext::GetAllocator(L);
    Rtt_UNUSED(allocator);

    lua_getfield(L, optionsIndex, "numFrames");
    int numFrames = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);

    const Texture &texture = fResource->GetTexture();
    int textureW = texture.GetWidth();
    int textureH = texture.GetHeight();

    // Content scaling
    lua_getfield(L, optionsIndex, "sheetContentWidth");
    int textureContentW = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, optionsIndex, "sheetContentHeight");
    int textureContentH = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);

    Real sx = Rtt_REAL_1;
    Real sy = Rtt_REAL_1;

    if (textureContentW > Rtt_REAL_0 && textureContentH > Rtt_REAL_0) {
        sx = Rtt_RealDiv(Rtt_IntToReal(textureW), Rtt_IntToReal(textureContentW));
        sy = Rtt_RealDiv(Rtt_IntToReal(textureH), Rtt_IntToReal(textureContentH));
    } else { // No content scaling, so default to texture dimensions
        textureContentW = textureW;
        textureContentH = textureH;
    }

    bool intrudeHalfTexel = fResource->GetTextureFactory().GetDisplay().GetDefaults().IsImageSheetSampledInsideFrame();
    if (numFrames > 0) { // Simple case
        lua_getfield(L, optionsIndex, "width");
        int frameW = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, optionsIndex, "height");
        int frameH = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, optionsIndex, "border");
        int border = Rtt::Max<int>(0, (int)lua_tointeger(L, -1));  // Ensure border >= 0
        lua_pop(L, 1);

        // Ensure a single frame has non-zero size and can fit in the texture.
        if ((frameW > 0 && frameW <= textureContentW) && (frameH > 0 && frameH <= textureContentH)) {
            // How far to advance x,y accounting for the border width
            int dx = frameW + 2 * border;
            int dy = frameH + 2 * border;
            S32 x = border;
            S32 y = border;

            // Generate frames
            for (int i = 0; i < numFrames; i++) {
                // Verify next frame will fit in image (base case checked above)
                if ((y + frameH) > textureContentH) {
                    // Lua is 1-based so (i+1)th frame.
                    luaL_error(L, "Incorrect number of frames (w,h) = (%d,%d) with border (%d) in texture (w,h) = (%d,%d). Failed after frame %d out of %d.", frameW, frameH, border, textureContentW, textureContentH, (i + 1), numFrames);
                    break;
                }

                fFrames.Append(Rtt_NEW(allocator, ImageFrame(*this, x, y, frameW, frameH, sx, sy, intrudeHalfTexel)));
                x += dx;

                // x already accounts for the left border width (b/c we init'd it to 'border')
                // so make sure there's enough room for the frameW and the right border.
                if ((x + frameW + border) > textureContentW) {
                    x = border;  // New row
                    y += dy;
                }
            }
        } else {
            luaL_argerror(L, optionsIndex, "for single frame size, 'options' table must contain valid 'width' and 'height' values");
        }
    } else { // Complex case
        lua_getfield(L, optionsIndex, "frames");
        bool hasFrames = lua_istable(L, -1);
        lua_pop(L, 1);

        if (hasFrames) {
            lua_getfield(L, optionsIndex, "frames");

            int framesIndex = lua_gettop(L);
            for (int i = 0, iMax = (int)lua_objlen(L, framesIndex); i < iMax; i++) {
                int index = (i + 1);  // Lua is 1-based so (i+1)th frame.
                lua_rawgeti(L, framesIndex, index);
                {
                    int element = lua_gettop(L);
                    if (lua_istable(L, element)) {
                        lua_getfield(L, element, "x");
                        S32 x = (S32)lua_tointeger(L, -1);

                        lua_getfield(L, element, "y");
                        S32 y = (S32)lua_tointeger(L, -1);

                        lua_getfield(L, element, "width");
                        S32 frameW = (S32)lua_tointeger(L, -1);

                        lua_getfield(L, element, "height");
                        S32 frameH = (S32)lua_tointeger(L, -1);

                        lua_pop(L, 4);

                        fFrames.Append(Rtt_NEW(allocator, ImageFrame(*this, x, y, frameW, frameH, sx, sy, intrudeHalfTexel)));
                    } else {
                        luaL_error(L, "for multiple frame sizes, 'options' should contain an numerically-ordered array of tables. However, element %d, i.e. options[%d], was not a table.", index, index);
                    }
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
    }

    Rtt_ASSERT(lua_gettop(L) == top);

    return 0;
}

// ----------------------------------------------------------------------------
}  // namespace Rtt
// ----------------------------------------------------------------------------
