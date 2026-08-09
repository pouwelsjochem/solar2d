//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_MacViewCallback_H__
#define _Rtt_MacViewCallback_H__

#include "Rtt_MCallback.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class Runtime;

// ----------------------------------------------------------------------------

class MacViewCallback : public MCallback
{
	public:
		MacViewCallback();

	public:
		void Initialize( Runtime *runtime ) { fRuntime = runtime; }
	
	public:
		virtual void operator()();

	private:
		Runtime *fRuntime;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MacViewCallback_H__
