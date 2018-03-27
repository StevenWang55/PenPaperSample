/*=============================================================================
Copyright (C) 2018 ACE CAD Enterprise Co., Ltd., All Rights Reserved.

Module Name:
PenPaper Sample Code for Windows Application

File Name:
	stdafx.h : include file for standard system include files,
			   or project specific include files that are used frequently,
			   but are changed infrequently
=============================================================================*/
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#include <afxdisp.h>        // MFC Automation classes

#include <MMSystem.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

#include <initguid.h>
#include <cfgmgr32.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <bthledef.h>
#include <Bluetoothleapis.h>


#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


//==================
// Common definition
//==================
#define PENPAPER_WIDTH	4970	// Width of the PenPaper in 1000 LPI unit.
#define PENPAPER_HEIGHT	6958	// Height of the PenPaper in 1000 LPI unit.

#define RET_OK			0
#define RET_ENUM_FAIL	1
#define RET_FAIL		2

#define SCREEN_DPI	96

#define MESSAGE_AREA_WIDTH	200	// Area for button and message, at left side of the draw area, unit in pixel

#define DRAW_AREA_WIDTH		PENPAPER_WIDTH * 96 / 1000		// 4970 = 4.97 inch, 4.97 inch * 96 DPI = 477 pixel
#define DRAW_AREA_HEIGHT	PENPAPER_HEIGHT * 96 / 1000		// 6958 = 6.958 inch, 6.958 inch * 96 DPI = 667 pixel
#define DRAW_AREA_LEFT		MESSAGE_AREA_WIDTH
#define DRAW_AREA_TOP		10

#define APP_CONTEXT_WIDTH	MESSAGE_AREA_WIDTH + DRAW_AREA_WIDTH + 20 + 2
#define APP_CONTEXT_HEIGHT	DRAW_AREA_TOP + DRAW_AREA_HEIGHT + 40 + 2

#define CONNECT_DRIVER_BUTTON_ID	1001
#define CONNECT_DEVICE_BUTTON_ID	1002

#define	COLOR_BLACK			RGB(  0,   0,   0)
#define	COLOR_BLUE			RGB(  0,   0, 255)
#define	COLOR_RED			RGB(255,   0,   0)
#define	COLOR_GREEN			RGB(  0, 255,   0)
#define	COLOR_CYAN			RGB(  0, 255, 255)

#include <pshpack1.h>
//====================================================
// For Method 1: Connect to PenPaper HID Mini Driver
//====================================================
/*---------------------------------*/
/* Control Data for App to Driver. */
/*---------------------------------*/
typedef struct _PENPAPER_DRIVER_CONTROL_SET
{
	UCHAR	ReportID;
	UCHAR	ControlCode;

	UCHAR	UCHAR_DATA_1;
	UCHAR	UCHAR_DATA_2;

	USHORT	USHORT_DATA_1;
	USHORT	USHORT_DATA_2;
	USHORT	USHORT_DATA_3;
	USHORT	USHORT_DATA_4;
} PENPAPER_DRIVER_CONTROL_SET, *PPENPAPER_DRIVER_CONTROL_SET;

/*-----------------------------------------------*/
/* Report Data for PenPaper Writing Mode to APP. */
/*-----------------------------------------------*/
typedef struct _PENPAPER_WRITING_MODE_DATA_FOR_APP
{
	BYTE	ReportID;

	BYTE	DeviceID;

	struct
	{
		BYTE TipSwitch : 1;
		BYTE Eraser : 1;
		BYTE BarrelSwitch : 1;
		BYTE InRange : 1;
		BYTE Invert : 1;
		BYTE Padding : 3;
	} Status;

	USHORT	X_coor;
	USHORT	Y_coor;
	USHORT	PressureValue;
} PENPAPER_WRITING_MODE_DATA_FOR_APP, *PPENPAPER_WRITING_MODE_DATA_FOR_APP;

/*--------------------------------*/
/* Struct for Read Report Thread. */
/*--------------------------------*/
typedef struct _ReadReportThreadInfo
{
	HANDLE	m_EventStart;
	HANDLE	m_EventStop;
	HANDLE	m_EventKill;
	HANDLE	m_EventThreadKilled;

	HWND	h_MainView;
	HANDLE	hPenPaperHID;
	int		i_IntCount;
} ReadReportThreadInfo, *PReadReportThreadInfo;

#include <poppack.h>

#define ORIENTATION_LANDSCAPE_RIGHT_HANDED	0
#define ORIENTATION_PORTRAIT				1
#define ORIENTATION_LANDSCAPE_LEFT_HANDED	2

/*--------------------*/
/*Driver Working mode */
/*--------------------*/
#define MODE_PEN_FUNCTION	0x00
#define MODE_WRITING		0x01
#define MODE_NEITHER		0xff

/*--------------------------*/
/* Driver Interface Command */
/*--------------------------*/
#define COMMAND_ENABLE_PEN_FUNCTION		0x01
#define COMMAND_DISABLE_PEN_FUNCTION	0x02

#define COMMAND_ENABLE_WRITING_MODE		0x03
#define COMMAND_DISABLE_WRITING_MODE	0x04

#define COMMAND_GET_TRACKING_AREA	0x05
#define COMMAND_SET_TRACKING_AREA	0x06
// UCHAR_DATA_1:	Orientation
// USHORT_DATA_1:	Tracking Area Left Margin
// USHORT_DATA_2:	Tracking Area Top Margin
// USHORT_DATA_3:	Tracking Area Width
// USHORT_DATA_4:	Tracking Area Height
#define COMMAND_GET_ORIENTATION		0x07
#define COMMAND_SET_ORIENTATION		0x08

#define COMMAND_SET_PRESSURE_LEVEL	0x09
// UCHAR_DATA_1:	Pressure Adjust Level
// UCHAR_DATA_2:	Position of the first value in the pressure table
//					There are 1024 values for each pressure table
// USHORT_DATA_1:	Pressure Value 1
// USHORT_DATA_2:	Pressure Value 2
// USHORT_DATA_3:	Pressure Value 3
// USHORT_DATA_4:	Pressure Value 4

#define COMMAND_SET_CLICK_THRESHOLD	0x0A
// UCHAR_DATA_1:	Click Threshold Level
// USHORT_DATA_1:	Tip On Threshold
// USHORT_DATA_2:	Tip Off Threshold

#define COMMAND_SET_FEATURE_NUMBER_RETURNED 0x0B
// UCHAR_DATA_1:	The set number of the Feature content that returned by the IOCTL_UMDF_HID_GET_FEATURE

// For IOCTL_UMDF_HID_GET_FEATURE, The definition of the struct:
// Set 1
// ControlCode:		1. The Set number for the content (For the host to check)
// UCHAR_DATA_1:	Major version of the driver
// UCHAR_DATA_2:	Minor version of the driver
// USHORT_DATA_1:	Reserved
// USHORT_DATA_2:	Reserved
// USHORT_DATA_3:	Reserved
// USHORT_DATA_4:	Reserved
// Set 2
// ControlCode:		The last command from the application by IOCTL_UMDF_HID_SET_FEATURE. <-- No use
// ControlCode:		2. The Set number for the content (For the host to check)
// UCHAR_DATA_1:	Current Orientation
// UCHAR_DATA_2:	Connection state of the PenPaper
// USHORT_DATA_1:	Working area left margin
// USHORT_DATA_2:	Working area top margin
// USHORT_DATA_3:	Width of the working area
// USHORT_DATA_4:	Height of the working area

#define WM_REPORT_DATA_READY (WM_USER + 1)

UINT ReadReportThreadProc(LPVOID pParam);

//===============================================================================
// For Method 2: Connect to PenPaper Bluetooth LE Device's Writing Mode service
//===============================================================================
#define WM_PENPAPER_DATA_READY (WM_USER + 2)

VOID CALLBACK s_ValueChangeEvent(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context);

//=================================================================================================
//=================================================================================================

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


