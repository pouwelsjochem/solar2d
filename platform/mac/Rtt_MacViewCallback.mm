//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_MacViewCallback.h"

#include "Rtt_Runtime.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

MacViewCallback::MacViewCallback()
:	fRuntime( NULL )
{
}

void
MacViewCallback::operator()()
{
	Rtt_ASSERT( fRuntime );
	(*fRuntime)();
	if ( fRuntime->IsProperty( Runtime::kRenderAsync ) )
	{
		fRuntime->Render();
	}
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------
