/*=============================================================================
Copyright (C) 2018 ACE CAD Enterprise Co., Ltd., All Rights Reserved.

Module Name:
	PenPaper Sample Code for Windows Application

File Name:
	PenPaperSample.cpp : Defines the class behaviors for the application.
=============================================================================*/
#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "PenPaperSample.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPenPaperSampleApp

BEGIN_MESSAGE_MAP(CPenPaperSampleApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CPenPaperSampleApp::OnAppAbout)
END_MESSAGE_MAP()


// CPenPaperSampleApp construction

CPenPaperSampleApp::CPenPaperSampleApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("PenPaperSample.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CPenPaperSampleApp object

CPenPaperSampleApp theApp;


// CPenPaperSampleApp initialization

BOOL CPenPaperSampleApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("PenPaper Sample Code"));


	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;
	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	
	// The one and only window has been initialized, so show and update it
	pFrame->ShowWindow(SW_SHOW);
	
	int XSizeFrame = GetSystemMetrics(SM_CXSIZEFRAME);
	int YSizeFrame = GetSystemMetrics(SM_CYSIZEFRAME);
	int CaptionHeight = GetSystemMetrics(SM_CYCAPTION);
	int MenubarHeight = GetSystemMetrics(SM_CYMENU);
	pFrame->SetWindowPos(&CWnd::wndTop,
		(GetSystemMetrics(SM_CXSCREEN) - (APP_CONTEXT_WIDTH + XSizeFrame * 2)) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - (APP_CONTEXT_HEIGHT + CaptionHeight + MenubarHeight + YSizeFrame * 2)) / 2,
		APP_CONTEXT_WIDTH + XSizeFrame * 2,
		APP_CONTEXT_HEIGHT + CaptionHeight + MenubarHeight + YSizeFrame * 2,
		SWP_SHOWWINDOW | SWP_NOZORDER);

	pFrame->UpdateWindow();
	return TRUE;
}

int CPenPaperSampleApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	AfxOleTerm(FALSE);

	return CWinApp::ExitInstance();
}

// CPenPaperSampleApp message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CPenPaperSampleApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CPenPaperSampleApp message handlers



