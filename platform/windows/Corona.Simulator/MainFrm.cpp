//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <gdiplus.h>
using namespace Gdiplus;

#include "Simulator.h"
#include "MainFrm.h"
#include "math.h"     // floor
#include "float.h"
#include "Core/Rtt_Build.h"
#include "Rtt_PlatformPlayer.h"
#include "Rtt_PlatformSimulator.h"
#include "Rtt_RenderingStream.h"
#include "Rtt_Runtime.h"
#include "SimulatorDoc.h"
#include "SimulatorView.h"

#define log2f(x) (logf(x)/logf(2.0f))

// The #includes must go above this line because this macro will override the "new" operator
// which will cause compiler errors with header files belonging to other libraries.
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
{
	// http://software.intel.com/en-us/articles/fast-floating-point-to-integer-conversions/
//	unsigned int control_word;
//	_controlfp_s( &control_word, 0, 0 );
//	_controlfp_s( &control_word, _RC_CHOP, _MCW_RC );
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

// PreCreateWindow - remove maximize box and resize frame
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= WS_CLIPCHILDREN;  // added for CView smaller than MainFrame.

	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

    // Can't maximize or resize window
	cs.style &= ~(WS_MAXIMIZEBOX|WS_THICKFRAME);

	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG

// CMainFrame message handlers

// OnClose - Save position of window at closing for next invocation 
void CMainFrame::OnClose()
{
	CSimulatorApp *app=(CSimulatorApp *)AfxGetApp();
    CSimulatorView *pView = (CSimulatorView *)GetActiveView();
	WINDOWPLACEMENT wp;

    // Store windows position
	this->GetWindowPlacement(&wp);
	app->PutWP(wp);
    app->PutDisplayName( pView->GetDisplayName() );

	// Stop simulation. This also posts user feedback to the server if enabled.
	pView->StopSimulation();

	// Close the window.
	CFrameWnd::OnClose();
}

/// Updates the title bar text on the window according to the currently selected document.
/// @param bAddToTitle Set TRUE to show the document name in the title.
///                    Set FALSE to only show the application name in the title.
void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	// Override the title bar text to only show the application name if we are currently
	// showing an internal screen such as the home screen or demos screen.
	CSimulatorView *viewPointer = (CSimulatorView*)GetActiveView();
	if (viewPointer && viewPointer->IsShowingInternalScreen())
	{
		bAddToTitle = FALSE;
	}
	
	// Do not display an document name in title bar if document does not have a title.
	// This works-around issue where MFC shows "- Corona Simulator" in this case.
	CDocument *documentPointer = GetActiveDocument();
	if (documentPointer && documentPointer->GetTitle().IsEmpty())
	{
		bAddToTitle = FALSE;
	}
	
	// Update the title bar text.
	CFrameWnd::OnUpdateFrameTitle(bAddToTitle);
}

// OnSize - override because we don't want to call CFrameWnd implementation
void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    // Skip CFrameWnd version, which ruins our layout
	CWnd::OnSize(nType, cx, cy);

	// We need to repaint the menu bar when the window grows in size
	// or else white space will be left behind.
	DrawMenuBar();
}

// OnGetMinMaxInfo - store the current min and max tracking sizes for zoom limits
// Max window sizes are limited by screen size, and min sizes are limited by menu size
void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// Increase the maximum size of the window to the biggest possible value.
	lpMMI->ptMaxTrackSize.x = LONG_MAX;
	lpMMI->ptMaxTrackSize.y = LONG_MAX;
}

// SizeToClient - resize the window to have the requested client size.
// CalcWindowRect is not reliable, so do the calculations here
// Don't redraw until later to avoid flickering (FALSE arg to MoveWindow).
// Return previous window rect, to be erased/redrawn later
CRect CMainFrame::SizeToClient( CRect rectNew )
{
	CRect rectWindow, rectClient;
    GetWindowRect( rectWindow );
    GetClientRect( rectClient );

    CRect rectPrevious = rectWindow;

	// Calculate non-client width & height (menu, frame, titlebar)
    int ncWidth = rectWindow.Width() - rectClient.Width();
    int ncHeight = rectWindow.Height() - rectClient.Height();

    rectWindow.right = rectWindow.left + ncWidth + rectNew.Width();
    rectWindow.bottom = rectWindow.top + ncHeight + rectNew.Height();

	MoveWindow( rectWindow, FALSE );

    GetWindowRect( rectWindow );
    GetClientRect( rectClient );

	// Again, calculate non-client width & height (menu, frame, titlebar)
    int ncNewWidth = rectWindow.Width() - rectClient.Width();
    int ncNewHeight = rectWindow.Height() - rectClient.Height();

    // If non-client area changed size (menu on 2 lines), try again
    if ((ncNewHeight != ncHeight) || (ncNewWidth != ncWidth))
	{
        // return largest possible previous rect to be erased/redrawn later
        rectPrevious.UnionRect( rectPrevious, rectWindow );

		rectWindow.right = rectWindow.left + ncNewWidth + rectNew.Width();
		rectWindow.bottom = rectWindow.top + ncNewHeight + rectNew.Height();

		MoveWindow( rectWindow, FALSE );
	}

    return rectPrevious;
}