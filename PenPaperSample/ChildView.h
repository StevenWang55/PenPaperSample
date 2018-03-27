/*=============================================================================
Copyright (C) 2018 ACE CAD Enterprise Co., Ltd., All Rights Reserved.

Module Name:
	PenPaper Sample Code for Windows Application

File Name:
	ChildView.h : interface of the CChildView class
=============================================================================*/
#pragma once

// CChildView window

class CChildView : public CWnd
{
// Construction
public:
	CChildView();

// Attributes
public:

// Operations
public:

// Overrides
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CChildView();

	// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnBnClickedConnectToDriver(void);
	afx_msg void OnBnClickedConnectToDevice(void);
	DECLARE_MESSAGE_MAP()

public:
	CButton cb_ConnectDriverButton;
	CButton cb_ConnectDeviceButton;

	// For GDI Plus
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	//--------------------------------------------
	// Variable to display the message and status.
	//--------------------------------------------
	CString	csString;
	int infoX;
	int infoY;

	//====================================================
	// For Scenario 1: Connect to PenPaper HID Mini Driver
	//====================================================
	int ConnectToPenPaperDriver(DWORD * dw_error);
	void SetFeatureNumberToGet(int i_FeatureNumber);
	int GetDriverStatus();
	LRESULT OnReportDataReady(WPARAM wParam, LPARAM lParam);

	HANDLE hPenPaperHID;
	PENPAPER_DRIVER_CONTROL_SET DriverStatusInfo;

	CWinThread* ReadReportThread;
	ReadReportThreadInfo m_ReadReportThreadInfo;
	HANDLE m_EventStart;
	HANDLE m_EventKill;
	HANDLE m_EventThreadKilled;
	BOOL b_TimerPeriodSetted;
	UINT workingMode;

	//===============================================================================
	// For Scenario 2: Connect to PenPaper Bluetooth LE Device's Writing Mode service
	//===============================================================================
	int ConnectToPenPaperDevice(DWORD * dw_error);
	LRESULT CChildView::OnPenPaperDataReady(WPARAM wParam, LPARAM lParam);

	HANDLE hPenPaper;
	BLUETOOTH_GATT_EVENT_HANDLE eventHandle;
	PBTH_LE_GATT_DESCRIPTOR pDescriptorBuffer;

	//================================================
	// Common routine and variables for both scenarios
	//================================================
	int IsPenPaperHIDDriverExist(HANDLE *hPenPaperHID);
	void InitPenPaperDrawingArea(void);
	void ClearMessageArea(void);
	void ShowPenPaperData(void);

	bool b_PenPaperHIDDriverExist;

	int i_pressureDivider;
	
	REAL drawAreaScaleX;
	REAL drawAreaScaleY;
	REAL PenPaperdrawAreaLeft;
	REAL PenPaperdrawAreaTop;

	CPen *pCursorPen;
	LOGBRUSH lbCursor;
	
	bool b_redPointShowing;

	int i_count;
	int pressure;
	int x_coor;
	int y_coor;
	int button;
	bool b_Proximity;
};

