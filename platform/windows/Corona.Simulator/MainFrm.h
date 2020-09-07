//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once


class CMainFrame : public CFrameWnd
{
	protected:
		CMainFrame();
		DECLARE_DYNCREATE(CMainFrame)

	public:
		virtual ~CMainFrame();
		virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
#ifdef _DEBUG
		virtual void AssertValid() const;
		virtual void Dump(CDumpContext& dc) const;
#endif
		CRect SizeToClient(CRect rectClient);

	protected:
		DECLARE_MESSAGE_MAP()
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnClose();
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

		void OnUpdateFrameTitle(BOOL bAddToTitle);
};
