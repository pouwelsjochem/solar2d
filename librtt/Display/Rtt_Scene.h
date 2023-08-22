//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_Scene_H__
#define _Rtt_Scene_H__

#include <set>

#include "Core/Rtt_Types.h"
#include "Display/Rtt_Paint.h"
#include "Display/Rtt_StageObject.h"
#include "Renderer/Rtt_CPUResource.h"
#include "Rtt_LuaUserdataProxy.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class Display;
class PlatformSurface;
class Profiling;
class Runtime;

// ----------------------------------------------------------------------------

// The Scene contains the tree of DisplayObject nodes, rooted in the StageObject.
//
// It also is the placeholder for various temporary collections. This includes
// where objects go when they are removed from the scene, i.e. the Orphanage.
//
// This is the top-level interface for rendering the entire DisplayObject tree.
class Scene
{
	public:
		Scene( Rtt_Allocator* pAllocator, Display& owner );
		~Scene();

	public:
//		GroupObject& Root() { return fRoot; }

	public:
//		Rtt_FORCE_INLINE const StageObject* operator->() const { return & fStage; }
//		Rtt_FORCE_INLINE StageObject* operator->() { return & fStage; }

		StageObject& CurrentStage() { return *fCurrentStage; }
		StageObject& OffscreenStage() { return *fOffscreenStage; }
		const Display& GetDisplay() const { return fOwner; }
		Display& GetDisplay() { return fOwner; }

		GroupObject& Orphanage() { return *fOrphanage; }
		GroupObject& SnapshotOrphanage() { return *fSnapshotOrphanage; }

		// Collects various resources passed into QueueRelease() after every 3rd and 32nd call.
		void Collect();

		// Collects all resources passed into QueueRelease() right now.
		// Intended to be called when the app is about to lose the OpenGL context and must delete GPU resources now.
		void ForceCollect();

	private:
		void OnCollectUnreachables();

	public:
		void QueueRelease( CPUResource *resource );
		void QueueRelease( LuaUserdataProxy *proxy );

	public:
		bool IsValid() const;
		void Invalidate();
		void Clear( Renderer& renderer );
		void Render( Renderer& renderer, PlatformSurface& rTarget, Profiling* profiling = NULL );
		void Render( Renderer& renderer, PlatformSurface& rTarget, DisplayObject& object );

	public:
		StageObject* PushStage();
		void PopStage();

	private:
		Display& fOwner;
		PtrArray< CPUResource > *fFrontResourceOrphanage;
		PtrArray< CPUResource > *fBackResourceOrphanage;
		StageObject *fCurrentStage; // top of stack of StageObjects
		StageObject *fOffscreenStage;
		StageObject *fOrphanage; // For Lua-created display objects removed from a group
		StageObject *fSnapshotOrphanage;
		LightPtrArray< LuaUserdataProxy > fProxyOrphanage;
		bool fIsValid;
		U8 fCounter; // DO NOT change type --- must be U8
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_Scene_H__
