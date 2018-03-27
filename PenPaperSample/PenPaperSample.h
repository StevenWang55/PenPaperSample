
// PenPaperSample.h : main header file for the PenPaperSample application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CPenPaperSampleApp:
// See PenPaperSample.cpp for the implementation of this class
//

class CPenPaperSampleApp : public CWinApp
{
public:
	CPenPaperSampleApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CPenPaperSampleApp theApp;
