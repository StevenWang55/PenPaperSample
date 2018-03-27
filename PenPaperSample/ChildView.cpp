/*=============================================================================
Copyright (C) 2018 ACE CAD Enterprise Co., Ltd., All Rights Reserved.

Module Name:
	PenPaper Sample Code for Windows Application

File Name:
	ChildView.cpp : implementation of the CChildView class
=============================================================================*/
#include "stdafx.h"
#include "PenPaperSample.h"
#include "ChildView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

CChildView::CChildView()
{
	REAL drawAreaScaleInvertX;
	REAL drawAreaScaleInvertY;

	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	//---------------------------------
	// Initial variables for method 1
	//---------------------------------
	m_EventStart = CreateEvent(NULL, FALSE, FALSE, NULL); // auto reset, initially rese
	m_EventKill = CreateEvent(NULL, FALSE, FALSE, NULL); // auto reset, initially rese
	m_EventThreadKilled = CreateEvent(NULL, FALSE, FALSE, NULL); // auto reset, initially rese
	m_ReadReportThreadInfo.m_EventStart = m_EventStart;
	m_ReadReportThreadInfo.m_EventKill = m_EventKill;
	m_ReadReportThreadInfo.m_EventThreadKilled = m_EventThreadKilled;
	ReadReportThread = NULL;

	workingMode = MODE_PEN_FUNCTION;
	hPenPaperHID = INVALID_HANDLE_VALUE;

	//---------------------------------
	// Initial variables for method 2
	//---------------------------------
	hPenPaper = NULL;
	eventHandle = NULL;

	//-------------------------
	// Initial common variables
	//-------------------------
	i_pressureDivider = 25;
	b_TimerPeriodSetted = FALSE;

	drawAreaScaleX = (REAL)DRAW_AREA_WIDTH / (REAL)PENPAPER_WIDTH;
	drawAreaScaleY = (REAL)DRAW_AREA_HEIGHT / (REAL)PENPAPER_HEIGHT;
	drawAreaScaleInvertX = (REAL)PENPAPER_WIDTH / (REAL)(DRAW_AREA_WIDTH);
	drawAreaScaleInvertY = (REAL)PENPAPER_HEIGHT / (REAL)(DRAW_AREA_HEIGHT);
	PenPaperdrawAreaLeft = (DRAW_AREA_LEFT + 2) * drawAreaScaleInvertX;
	PenPaperdrawAreaTop = (DRAW_AREA_TOP + 2) * drawAreaScaleInvertY;

	pCursorPen = new CPen;
	lbCursor.lbColor = COLOR_CYAN; lbCursor.lbStyle = BS_SOLID; lbCursor.lbHatch = 0;
	pCursorPen->CreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND, 4, &lbCursor);

	//---------------------------------------------------------------------------------------
	// Check and get the Interface Handle of the PenPaper HID minidriver if it exist.
	// If the PenPaper HID minidriver exist, then we can only use the method 1 to get the
	// data from PenPaper through the driver.
	// If the PenPaper HID minidriver NOT exist, then we must connect to the PenPaper through
	// the Bluetooth GATT functions. That's the way of the method 2.
	//---------------------------------------------------------------------------------------
	if (RET_OK == IsPenPaperHIDDriverExist(&hPenPaperHID)) b_PenPaperHIDDriverExist = true;
	else b_PenPaperHIDDriverExist = false;
}

CChildView::~CChildView()
{
	ULONG bufferSize;
	PENPAPER_DRIVER_CONTROL_SET controlSet;

	if (b_PenPaperHIDDriverExist == true)
	{
		if (b_TimerPeriodSetted == TRUE)
		{
			timeEndPeriod(1);
			b_TimerPeriodSetted = FALSE;
		}

		if (ReadReportThread != NULL)
		{
			SetEvent(m_EventKill);
			WaitForSingleObject(m_EventThreadKilled, 3000);
			ReadReportThread = NULL;
		}

		if (hPenPaperHID != INVALID_HANDLE_VALUE)
		{
			//---------------------------------------
			// Switch the driver to Pen-Function-Mode
			//---------------------------------------
			if (workingMode == MODE_WRITING)
			{
				bufferSize = sizeof(PENPAPER_DRIVER_CONTROL_SET);
				ZeroMemory(&controlSet, bufferSize);
				controlSet.ReportID = 2;
				controlSet.ControlCode = COMMAND_ENABLE_PEN_FUNCTION;
				HidD_SetFeature(hPenPaperHID, &controlSet, bufferSize);
				workingMode = MODE_PEN_FUNCTION;
			}

			CloseHandle(hPenPaperHID);
		}

		b_PenPaperHIDDriverExist = false;
	}

	if (eventHandle != NULL) BluetoothGATTUnregisterEvent(eventHandle, BLUETOOTH_GATT_FLAG_NONE);
	
	if (hPenPaper != NULL)
	{
		/*----------------------------------------------*/
		/* Clear notification flag to Descriptor Value. */
		/*----------------------------------------------*/
		BTH_LE_GATT_DESCRIPTOR_VALUE newValue;

		RtlZeroMemory(&newValue, sizeof(newValue));
		newValue.DescriptorType = ClientCharacteristicConfiguration;
		newValue.ClientCharacteristicConfiguration.IsSubscribeToNotification = FALSE;
		BluetoothGATTSetDescriptorValue(hPenPaper, pDescriptorBuffer, &newValue, BLUETOOTH_GATT_FLAG_NONE);

		CloseHandle(hPenPaper);
	}

	if (pCursorPen != NULL) pCursorPen->DeleteObject();
	GdiplusShutdown(gdiplusToken);
}

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_BN_CLICKED(CONNECT_DRIVER_BUTTON_ID, &CChildView::OnBnClickedConnectToDriver)
	ON_MESSAGE(WM_REPORT_DATA_READY, OnReportDataReady)
	ON_BN_CLICKED(CONNECT_DEVICE_BUTTON_ID, &CChildView::OnBnClickedConnectToDevice)
	ON_MESSAGE(WM_PENPAPER_DATA_READY, OnPenPaperDataReady)
END_MESSAGE_MAP()

// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	cb_ConnectDriverButton.Create(L"Connect to Driver", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, CRect(0, 0, 0, 0), this, CONNECT_DRIVER_BUTTON_ID);
	cb_ConnectDriverButton.MoveWindow(10, 40, 180, 36);

	cb_ConnectDeviceButton.Create(L"Connect to Device", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, CRect(0, 0, 0, 0), this, CONNECT_DEVICE_BUTTON_ID);
	cb_ConnectDeviceButton.MoveWindow(10, 100, 180, 36);

	//---------------------------------------------------------------------------------------------------------
	// If the PenPaper HID minidriver exist¡A then disable the button for connecting by Bluetooth GATT Function
	// If the PenPaper HID minidriver NOT exist¡A then disable the button for connecting by HID minidriver
	//---------------------------------------------------------------------------------------------------------
	if (b_PenPaperHIDDriverExist == true) cb_ConnectDeviceButton.EnableWindow(0);
	else cb_ConnectDriverButton.EnableWindow(0);

	return 0;
}

void CChildView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	InitPenPaperDrawingArea();
	
	// Do not call CWnd::OnPaint() for painting messages
}

//=====================================================================
//=====================================================================
static USAGE g_PenPaperHIDUsagePage = 0xFF01;
static USAGE g_PenPaperHIDUsage = 0x01;

int CChildView::IsPenPaperHIDDriverExist(HANDLE *hPenPaperHID)
{
	GUID hidguid;
	ULONG deviceInterfaceListLength = 0;
	PWSTR deviceInterfaceList = NULL;
	PWSTR currentInterface;
	PHIDP_PREPARSED_DATA Ppd;	// The opaque parser info describing this device
	HIDP_CAPS Caps;				// The Capabilities of this hid device.
	HIDD_ATTRIBUTES attr;		// Device attributes
	BOOL b_FindDevice = FALSE;

	CONFIGRET cr = CR_SUCCESS;
	DWORD dw_error;
	NTSTATUS ntStatus;

	//-------------------------------------------------------------
	// Step 1: Get PenPaper HID Minidriver Device Interface Handle. 
	//-------------------------------------------------------------
	HidD_GetHidGuid(&hidguid);

	cr = CM_Get_Device_Interface_List_Size(&deviceInterfaceListLength, &hidguid, NULL, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
	if (cr != CR_SUCCESS)
	{
		dw_error = GetLastError();
		*hPenPaperHID = INVALID_HANDLE_VALUE;
		return RET_FAIL;
	}

	deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
	if (deviceInterfaceList == NULL)
	{
		dw_error = GetLastError();
		*hPenPaperHID = INVALID_HANDLE_VALUE;
		return RET_FAIL;
	}
	ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

	cr = CM_Get_Device_Interface_List(&hidguid, NULL, deviceInterfaceList, deviceInterfaceListLength, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
	if (cr != CR_SUCCESS)
	{
		dw_error = GetLastError();
		*hPenPaperHID = INVALID_HANDLE_VALUE;
		return RET_FAIL;
	}

	int i = 0;
	for (currentInterface = deviceInterfaceList; *currentInterface; currentInterface += wcslen(currentInterface) + 1)
	{
		i++;

		*hPenPaperHID = CreateFile(currentInterface,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,									// no SECURITY_ATTRIBUTES structure
			OPEN_EXISTING,							// No special create flags
													//0,										// No special attributes
			FILE_FLAG_OVERLAPPED,
			NULL);									// No template file

		if (*hPenPaperHID != INVALID_HANDLE_VALUE)
		{
			if (!HidD_GetAttributes(*hPenPaperHID, &attr)) CloseHandle(*hPenPaperHID);
			else
			{
				if (attr.VendorID != 0x0460) CloseHandle(*hPenPaperHID);
				else
				{
					if (!HidD_GetPreparsedData(*hPenPaperHID, &Ppd)) CloseHandle(*hPenPaperHID);
					else
					{
						ntStatus = HidP_GetCaps(Ppd, &Caps);
						HidD_FreePreparsedData(Ppd);
						if (ntStatus != HIDP_STATUS_SUCCESS) CloseHandle(*hPenPaperHID);
						else
						{
							if ((Caps.UsagePage == g_PenPaperHIDUsagePage) && (Caps.Usage == g_PenPaperHIDUsage))
							{
								b_FindDevice = TRUE;
								break;
							}
							else CloseHandle(*hPenPaperHID);
						}
					}
				}
			}
		}
	}

	free(deviceInterfaceList);

	if (b_FindDevice == TRUE) return RET_OK;
	else return RET_FAIL;
}

//============================================================================
// Method 1: Connect to PenPaper HID Mini Driver
//============================================================================
void CChildView::OnBnClickedConnectToDriver(void)
{
	DWORD	dwError;
	int		i_Ret;

	InitPenPaperDrawingArea();
	ClearMessageArea();

	//-------------------------------------------------------------------------------------------------------
	// If the PenPaper is disconnected, the followinf procesure will wait about few seconds without response.
	//-------------------------------------------------------------------------------------------------------
	i_Ret = ConnectToPenPaperDriver(&dwError);
	if (i_Ret != RET_OK)
	{
		CDC* mainDC = GetDC();
		int MessageX = 210;
		int MessageY = 40;

		mainDC->TextOut(MessageX, MessageY, L"Fail to connect to the PenPaper HID Driver.");
		csString.Format(L"- Erroe Code: 0x%x", dwError);
		mainDC->TextOut(MessageX, MessageY + 20, csString);
		ReleaseDC(mainDC);
	}
	else
	{
		InitPenPaperDrawingArea();
		b_Proximity = false;
		x_coor = y_coor = 0;
		button = 0;
		pressure = 0;
		ShowPenPaperData();
	}
}

int CChildView::ConnectToPenPaperDriver(DWORD * dw_error)
{
	//------------------------------
	// Variable for display message.
	//------------------------------
	CDC*	mainDC = GetDC();

	ULONG bufferSize;
	PENPAPER_DRIVER_CONTROL_SET controlSet;

	infoX = 210;
	infoY = 40;

	if (b_PenPaperHIDDriverExist == false)
	{
		if (RET_OK == IsPenPaperHIDDriverExist(&hPenPaperHID)) b_PenPaperHIDDriverExist = true;
		else return RET_FAIL;
	}

	//--------------------------------------------
	// Get PenPaper HID Minidriver version number
	//--------------------------------------------
	SetFeatureNumberToGet(1);
	bufferSize = sizeof(PENPAPER_DRIVER_CONTROL_SET);
	ZeroMemory(&controlSet, bufferSize);
	controlSet.ReportID = 2;
	if (HidD_GetFeature(hPenPaperHID, &controlSet, bufferSize))
	{
		csString.Format(L"Driver Version: %d.%d", controlSet.UCHAR_DATA_1, controlSet.UCHAR_DATA_2);
		mainDC->TextOut(10, 10, csString);
	}

	//-----------------------------
	// Ready the Read-Report Thread
	//-----------------------------
	if (ReadReportThread == NULL)
	{
		m_ReadReportThreadInfo.h_MainView = this->m_hWnd;
		m_ReadReportThreadInfo.hPenPaperHID = hPenPaperHID;
		m_ReadReportThreadInfo.i_IntCount = 0;

		ReadReportThread = AfxBeginThread(ReadReportThreadProc, &m_ReadReportThreadInfo, THREAD_PRIORITY_TIME_CRITICAL, 0, 0, NULL);
		SetThreadPriority(ReadReportThread, THREAD_PRIORITY_TIME_CRITICAL);

		timeBeginPeriod(1);
		b_TimerPeriodSetted = TRUE;

		ResetEvent(m_EventKill);
		ResetEvent(m_EventThreadKilled);
		SetEvent(m_EventStart);
	}

	//----------------------------------
	// Switch the driver to Writing-Mode
	//----------------------------------
	if (workingMode == MODE_PEN_FUNCTION)
	{
		bufferSize = sizeof(PENPAPER_DRIVER_CONTROL_SET);
		ZeroMemory(&controlSet, bufferSize);
		controlSet.ReportID = 2;
		controlSet.ControlCode = COMMAND_ENABLE_WRITING_MODE;
		controlSet.UCHAR_DATA_1 = (UCHAR)ORIENTATION_PORTRAIT;
		HidD_SetFeature(hPenPaperHID, &controlSet, bufferSize);
		workingMode = MODE_WRITING;
	}

	ReleaseDC(mainDC);

	return RET_OK;
}

int CChildView::GetDriverStatus()
{
	ULONG bufferSize;
	
	if (b_PenPaperHIDDriverExist == false) return RET_FAIL;
	
	SetFeatureNumberToGet(2);

	bufferSize = sizeof(PENPAPER_DRIVER_CONTROL_SET);
	ZeroMemory(&DriverStatusInfo, bufferSize);
	
	DriverStatusInfo.ReportID = 2;
	if (HidD_GetFeature(hPenPaperHID, &DriverStatusInfo, bufferSize)) return RET_OK;
	else return RET_FAIL;
}

void CChildView::SetFeatureNumberToGet(int i_FeatureNumber)
{
	ULONG bufferSize;
	PENPAPER_DRIVER_CONTROL_SET controlSet;

	if (b_PenPaperHIDDriverExist == false) return;

	bufferSize = sizeof(PENPAPER_DRIVER_CONTROL_SET);
	ZeroMemory(&controlSet, bufferSize);

	controlSet.ReportID = 2;
	controlSet.ControlCode = COMMAND_SET_FEATURE_NUMBER_RETURNED;
	controlSet.UCHAR_DATA_1 = (UCHAR)i_FeatureNumber;
	HidD_SetFeature(hPenPaperHID, &controlSet, bufferSize);
}

LRESULT CChildView::OnReportDataReady(WPARAM wParam, LPARAM lParam)
{
	if (wParam & 0x0001) b_Proximity = true;
	else b_Proximity = false;
	x_coor = LOWORD(lParam);
	y_coor = PENPAPER_HEIGHT - HIWORD(lParam);
	button = (wParam >> 1) & 0x0007;
	pressure = wParam >> 4;
	i_count++;
	ShowPenPaperData();

	return LRESULT();
}

//--------------------------------
// Thread to read the report data.
//--------------------------------
UINT ReadReportThreadProc(LPVOID pParam)
{
	ReadReportThreadInfo* pInfo = (ReadReportThreadInfo*)pParam;
	static HANDLE hPenPaperHID;
	static OVERLAPPED osRead;
	static DWORD dwRead;
	static DWORD dwRes;
	static DWORD dwReturn;
	static DWORD dwLastError;
	static BOOL bReadResult;
	static unsigned char buffer[30];
	static UCHAR ucTemp;
	static WPARAM wParam;
	static ULONG lParam;
	static UINT proximity;
	static DWORD dataNumber;
	static PPENPAPER_WRITING_MODE_DATA_FOR_APP pPenPaperData;

	hPenPaperHID = pInfo->hPenPaperHID;

	//::PostMessage(pInfo->h_MainView, WM_DATA_ERROR, 255, 0);
	/*------------------------------------------------------------------------------*/
	/* At the beginning, the thread will stop and wait at the following instruction */
	/*------------------------------------------------------------------------------*/
	if (WaitForSingleObject(pInfo->m_EventStart, INFINITE) != WAIT_OBJECT_0)
	{
		SetEvent(pInfo->m_EventThreadKilled);
		return 0;
	}

	dataNumber = sizeof(PENPAPER_WRITING_MODE_DATA_FOR_APP);

	/*-----------------------*/
	/* Now, the thread start */
	/*-----------------------*/
	//b_dataProcessed = true;

	while (true)
	{
		if (WaitForSingleObject(pInfo->m_EventKill, 1) == WAIT_OBJECT_0) break;

		/*-----------------------------------------------------------------------------*/
		/* Read data from PenPaper Writing Mode To HID Device's Second Top Collection. */
		/*-----------------------------------------------------------------------------*/
		bReadResult = FALSE;
		osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!ReadFile(hPenPaperHID, &buffer, dataNumber, &dwRead, &osRead))
		{
			/* The read action do not complete immediately. */
			dwLastError = GetLastError();
			if (dwLastError == ERROR_IO_PENDING)
			{
				dwRes = WaitForSingleObject(osRead.hEvent, 500);	/* Wait for 500 ms. */
				if (dwRes == WAIT_OBJECT_0)
				{
					if (GetOverlappedResult(hPenPaperHID, &osRead, &dwRead, FALSE)) bReadResult = TRUE;
				}
				if (dwRes == WAIT_TIMEOUT) dwReturn = 3;	/* Timeout when receive data. */
				else dwReturn = 2;	/* Other errors in the WaitForSingleObject() when receive data. */
			}
			else dwReturn = 1;	/* Errors that others than IO_PENDING when receive data. */
		}
		else bReadResult = TRUE;

		if (dwRead != dataNumber) continue;

		if (bReadResult == TRUE)
		{
			pPenPaperData = (PPENPAPER_WRITING_MODE_DATA_FOR_APP)&buffer[0];
			proximity = pPenPaperData->Status.InRange;
			ucTemp = pPenPaperData->Status.BarrelSwitch << 2 | pPenPaperData->Status.Eraser << 1 | pPenPaperData->Status.TipSwitch;
			wParam = pPenPaperData->PressureValue << 4 | ucTemp << 1 | proximity;
			lParam = pPenPaperData->X_coor | pPenPaperData->Y_coor << 16;
			::PostMessage(pInfo->h_MainView, WM_REPORT_DATA_READY, wParam, lParam);

			pInfo->i_IntCount++;
		}
	} /* while(true) */

	SetEvent(pInfo->m_EventThreadKilled);

	return 0;
}



//=====================================================================================================================
// Method 2: Connect to PenPaper Bluetooth LE Device's Writing Mode service
//=====================================================================================================================
//------------------------------------------------------------------
// The GUIDs of the PenPaper Report-In Service and Characteristics. 
//------------------------------------------------------------------
static GUID PenPaperPenWritingServiceGuidLong =	{ 0x0000ace0, 0x0000, 0x1000,{ 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };
static GUID PenPaperReportCharacteristicGuid =	{ 0x0000ace0, 0x0001, 0x1000,{ 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };

void CChildView::OnBnClickedConnectToDevice(void)
{
	DWORD	dwError;
	int		i_Ret;

	InitPenPaperDrawingArea();
	ClearMessageArea();

	//-------------------------------------------------------------------------
	// If we have connected to the PenPaper by Bluetooth GATT functions before,
	// then we must un-register the event handler and free the handle first.
	//-------------------------------------------------------------------------
	if (eventHandle != NULL)
	{
		BluetoothGATTUnregisterEvent(eventHandle, BLUETOOTH_GATT_FLAG_NONE);
		eventHandle = NULL;
	}
	if (hPenPaper != NULL)
	{
		CloseHandle(hPenPaper);
		hPenPaper = NULL;
	}

	//-------------------------------------------------------------------------------------------------------
	// If the PenPaper is disconnected, the following procesure will wait about few seconds without response.
	//-------------------------------------------------------------------------------------------------------
	i_Ret = ConnectToPenPaperDevice(&dwError);
	if(i_Ret != RET_OK)
	{
		CDC* mainDC = GetDC();
		int MessageX = 10;
		int MessageY = 200;

		mainDC->TextOut(MessageX, MessageY, L"Connect PenPaper fail,");
		csString.Format(L"- Erroe Code: 0x%x", dwError);
		mainDC->TextOut(MessageX, MessageY+20, csString);
		ReleaseDC(mainDC);
	}
	else
	{
		InitPenPaperDrawingArea();
		b_Proximity = false;
		x_coor = y_coor = 0;
		button = 0;
		pressure = 0;
		ShowPenPaperData();
	}
}

int CChildView::ConnectToPenPaperDevice(DWORD * dw_error)
{
	//------------------------------
	// Variable for display message.
	//------------------------------
	CDC*	mainDC = GetDC();

	bool	b_FindPenPaperDevice;
	
	//--------------------------------
	// Variable for SetupAPI functions
	//--------------------------------
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInformationdata;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData = NULL;
	
	DWORD MemberIndex;
	DWORD InterfaceIndex;

	DWORD dwRequiredSize;
	DWORD bufferSize;
	DWORD dwError;
	PWSTR pUniTarget;

	//----------------------------------------
	// Variable for BluetoothGATTxxx functions
	//----------------------------------------
	HRESULT hr;

	infoX = 210;
	infoY = 40;

	//---------------------------------------------------------
	// Step 1: Get Handle to the PenPaper Writing Mode Service 
	//---------------------------------------------------------
	//hDevInfo = SetupDiGetClassDevs(&GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
	hDevInfo = SetupDiGetClassDevs(&GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	b_FindPenPaperDevice = false;
	DeviceInformationdata.cbSize = sizeof(SP_DEVINFO_DATA);
	MemberIndex = 0;

	while (SetupDiEnumDeviceInfo(hDevInfo, MemberIndex++, &DeviceInformationdata))
	{
		DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		InterfaceIndex = 0;

		//while (SetupDiEnumDeviceInterfaces(hDevInfo, &DeviceInformationdata, &PenPaperPenWritingServiceGuidLong, InterfaceIndex++, &DeviceInterfaceData))
		while (SetupDiEnumDeviceInterfaces(hDevInfo, &DeviceInformationdata, &GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, InterfaceIndex++, &DeviceInterfaceData))
		{
			//--------------------------------------
			// Get and allocate required buffer size 
			//--------------------------------------
			SetupDiGetInterfaceDeviceDetail(hDevInfo, &DeviceInterfaceData, NULL, 0, &dwRequiredSize, NULL);
			dwError = GetLastError();
			
			//------------------------------------------------------------------------------
			// If its not ERROR_INSUFFICIENT_BUFFER error, continue to enumerate next anyway.
			//------------------------------------------------------------------------------
			if (dwError != ERROR_INSUFFICIENT_BUFFER) continue;

			//-------------------------------
			// Free previous allocated buffer
			//-------------------------------
			if (pDeviceInterfaceDetailData != NULL) free(pDeviceInterfaceDetailData);

			
			pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(dwRequiredSize);
			
			//----------------------------------------------------------
			// For the malloc() error, continue to enumerate next anyway.
			//----------------------------------------------------------
			if (pDeviceInterfaceDetailData == NULL) continue;

			//---------------------------------
			// Get Interface Device Detail Data
			//---------------------------------
			bufferSize = dwRequiredSize;
			pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (SetupDiGetInterfaceDeviceDetail(hDevInfo, &DeviceInterfaceData, pDeviceInterfaceDetailData, bufferSize, &dwRequiredSize, NULL))
			{
				//--------------------------------------------------
				// Is it the PenPaper Writing Mode Service: 0000ace0
				//--------------------------------------------------
				pUniTarget = wcsstr(pDeviceInterfaceDetailData->DevicePath, L"0000ace0");
				if (pUniTarget != NULL)
				{
					//------------------------------------------
					// Create handle to the Writing Mode Service
					//------------------------------------------
					hPenPaper = CreateFile(pDeviceInterfaceDetailData->DevicePath,
						GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
						0,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
					if (hPenPaper != INVALID_HANDLE_VALUE)
					{
						//------------------------------------
						// Connect to the first one enumerated
						//------------------------------------
						b_FindPenPaperDevice = true;
						break;
					}
				}
			}
		}	// while (SetupDiEnumDeviceInterfaces()....
	}	// while (SetupDiEnumDeviceInfo()....

	*dw_error = GetLastError();

	if (pDeviceInterfaceDetailData != NULL) free(pDeviceInterfaceDetailData);
	
	SetupDiDestroyDeviceInfoList(hDevInfo);

	if (b_FindPenPaperDevice == false)
	{
		csString.Format(L"Enumerate PenPaper Writing Mode Service Device Fail.");
		mainDC->TextOut(infoX, infoY, csString);

		ReleaseDC(mainDC);
		return RET_ENUM_FAIL;
	}

	mainDC->TextOut(infoX, infoY, L"Find PenPaper Writing Mode Service Device.");
	infoY = infoY + 20;

	//--------------------------------------------------------------------
	// Step 2: Call Bluetooth GATT Functions to communicate with PenPaper
	//--------------------------------------------------------------------
	//---------------------------
	// Step 2-1: Retrieve Service
	//---------------------------
	PBTH_LE_GATT_SERVICE pServiceBuffer;
	USHORT serviceBufferCount;
	USHORT numServices;

	hr = BluetoothGATTGetServices(hPenPaper, 0, NULL, &serviceBufferCount, BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
	{
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"BluetoothGATTGetServices() Erroe (Get buffer size).");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	pServiceBuffer = (PBTH_LE_GATT_SERVICE)malloc(serviceBufferCount * sizeof(BTH_LE_GATT_SERVICE));

	if (pServiceBuffer == NULL)
	{
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"Allocate memory for GetService Erroe.");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	hr = BluetoothGATTGetServices(hPenPaper, serviceBufferCount, pServiceBuffer, &numServices, BLUETOOTH_GATT_FLAG_NONE);
	if (hr != S_OK)
	{
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"BluetoothGATTGetServices() fail.");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	mainDC->TextOut(infoX, infoY, L"Call BluetoothGATTGetServices() successful.");
	infoY = infoY + 20;

	//----------------------------------
	// Step 2-2: Retrieve Characteristic
	//----------------------------------
	PBTH_LE_GATT_CHARACTERISTIC pCharacteristicBuffer;
	PBTH_LE_GATT_CHARACTERISTIC pTargetCharacteristicBuffer;
	USHORT characteristicBufferCount;
	USHORT numCharacteristics;
	GUID *newGuid;
	int i_isSameGuid;

	hr = BluetoothGATTGetCharacteristics(hPenPaper, pServiceBuffer, 0, NULL, &characteristicBufferCount, BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
	{
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"BluetoothGATTGetCharacteristics() Erroe (Get buffer size).");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	pCharacteristicBuffer = (PBTH_LE_GATT_CHARACTERISTIC)malloc(characteristicBufferCount * sizeof(BTH_LE_GATT_CHARACTERISTIC));

	if (pCharacteristicBuffer == NULL)
	{
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"Allocate memory for GetCharacteristics Erroe.");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	hr = BluetoothGATTGetCharacteristics(hPenPaper, pServiceBuffer, characteristicBufferCount, pCharacteristicBuffer, &numCharacteristics, BLUETOOTH_GATT_FLAG_NONE);

	if (hr != S_OK)
	{
		free(pCharacteristicBuffer);
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"BluetoothGATTGetCharacteristics() fail.");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	i_isSameGuid = 0;
	for (int i = 0; i < numCharacteristics; i++)
	{
		pTargetCharacteristicBuffer = pCharacteristicBuffer + i;
		newGuid = (GUID *)&(pTargetCharacteristicBuffer->CharacteristicUuid.Value);
		i_isSameGuid = IsEqualGUID(PenPaperReportCharacteristicGuid, *newGuid);

		if (i_isSameGuid != 0) break;
	}

	if (i_isSameGuid == 0)
	{
		free(pCharacteristicBuffer);
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"Target characteristic not found.");
		mainDC->TextOut(infoX, infoY, csString);

		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	mainDC->TextOut(infoX, infoY, L"Find target GATT Characteristic.");
	infoY = infoY + 20;

	//------------------------------
	// Step 2-3: Retrieve Descriptor
	//------------------------------
	//PBTH_LE_GATT_DESCRIPTOR pDescriptorBuffer;
	USHORT descriptorBufferCount;
	USHORT numDescriptors;

	hr = BluetoothGATTGetDescriptors(hPenPaper, pTargetCharacteristicBuffer, 0, NULL, &descriptorBufferCount, BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
	{
		free(pCharacteristicBuffer);
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"BluetoothGATTGetDescriptors() Erroe (Get buffer size).");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	pDescriptorBuffer = (PBTH_LE_GATT_DESCRIPTOR)malloc(descriptorBufferCount * sizeof(BTH_LE_GATT_DESCRIPTOR));

	if (pDescriptorBuffer == NULL)
	{
		free(pCharacteristicBuffer);
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"Allocate memory for GetDescriptor Erroe.");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	hr = BluetoothGATTGetDescriptors(hPenPaper, pTargetCharacteristicBuffer, descriptorBufferCount, pDescriptorBuffer, &numDescriptors, BLUETOOTH_GATT_FLAG_NONE);

	if (hr != S_OK)
	{
		free(pDescriptorBuffer);
		free(pCharacteristicBuffer);
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"BluetoothGATTGetDescriptors() fail.");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	mainDC->TextOut(infoX, infoY, L"Get GATT Descriptor.");
	infoY = infoY + 20;

	//-------------------------------------------------------------------
	// Step 2-4: Register callback for Characteristics Value Change Event
	//-------------------------------------------------------------------
	BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION registration = { 0 };

	registration.NumCharacteristics = 1;
	registration.Characteristics[0] = *pTargetCharacteristicBuffer;
	hr = BluetoothGATTRegisterEvent(hPenPaper,
		CharacteristicValueChangedEvent,
		(PVOID)&registration,
		(PFNBLUETOOTH_GATT_EVENT_CALLBACK)s_ValueChangeEvent,
		(PVOID)this,
		&eventHandle,
		BLUETOOTH_GATT_FLAG_NONE);

	if (hr != S_OK)
	{
		free(pDescriptorBuffer);
		free(pCharacteristicBuffer);
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"BluetoothGATTGetDescriptors() fail.");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	mainDC->TextOut(infoX, infoY, L"Register event change callback.");
	infoY = infoY + 20;

	/*------------------------------------------------*/
	/* 2-5. Set notification flag to Descriptor Value. */
	/*------------------------------------------------*/
	BTH_LE_GATT_DESCRIPTOR_VALUE newValue;

	RtlZeroMemory(&newValue, sizeof(newValue));
	newValue.DescriptorType = ClientCharacteristicConfiguration;
	newValue.ClientCharacteristicConfiguration.IsSubscribeToNotification = TRUE;

	hr = BluetoothGATTSetDescriptorValue(hPenPaper, pDescriptorBuffer, &newValue, BLUETOOTH_GATT_FLAG_NONE);

	if (hr != S_OK)
	{
		free(pDescriptorBuffer);
		free(pCharacteristicBuffer);
		free(pServiceBuffer);
		CloseHandle(hPenPaper);
		hPenPaper = NULL;

		csString.Format(L"BluetoothGATTSetDescriptorValue() fail.");
		mainDC->TextOut(infoX, infoY, csString);

		*dw_error = GetLastError();
		ReleaseDC(mainDC);
		return RET_FAIL;
	}

	mainDC->TextOut(infoX, infoY, L"Set notification flag done.");

	//free(pDescriptorBuffer);		// Do not free pDescriptorBuffer here, we need it to clear the notification flag later
	free(pCharacteristicBuffer);
	free(pServiceBuffer);

	SetupDiDestroyDeviceInfoList(hDevInfo);
	ReleaseDC(mainDC);
	return RET_OK;
}

LRESULT CChildView::OnPenPaperDataReady(WPARAM wParam, LPARAM lParam)
{
	if(wParam & 0x0001)	b_Proximity = true;
	else b_Proximity = false;
	x_coor = LOWORD(lParam);
	y_coor = HIWORD(lParam);
	button = (wParam >> 1) & 0x0007;
	pressure = wParam >> 4;
	i_count++;
	ShowPenPaperData();

	return LRESULT();
}

/**********************************************************************************************
PenPaper Writing Mode Report Data Format :
-----------------------------------------------------------------------
Transmit	| MSB													LSB
-----------------------------------------------------------------------
			| 7		 6		 5		4		3		2		1		0
Byte 1		| 1		xxx		xxx		PR		B2		B1		TIP		1
Byte 2		| 0		X6		X5		X4		X3		X2		X1		X0
Byte 3		| 0		X13		X12		X11		X10		X9		X8		X7
Byte 4		| 0		Y6		Y5		Y4		Y3		Y2		Y1		Y0
Byte 5		| 0		Y13		Y12		Y11		Y10		Y9		Y8		Y7
Byte 6		| 0		R6		R5		R4		R3		R2		R1		R0
Byte 7		| 0		xxx		xxx		xxx		xxx		R9		R8		R7
-----------------------------------------------------------------------
	PR			: Proximity flag. 1 = In Proximity, 0 = Out of proximity.
	TIP			: Tip button status. 1 = On, 0 = Off.
	B1			: Lower barrel button status. 1 = On, 0 = Off.
	B2			: Upper barrel button status. 1 = On, 0 = Off.
	X0 ~ X13	: X axis coordinate value.
	Y0 ~ Y13	: Y axis coordinate value.
	R0 ~ R9		: Pressure value.Maximum level = 1023.
	xxx			: Not defined, reserved for future applications.
**********************************************************************************************/
VOID CALLBACK s_ValueChangeEvent(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{
	static WPARAM wParam;
	static ULONG lParam;
	static UINT TmpLow, TmpMiddle, TmpHigh;
	static int x_coor, y_coor;
	static UINT proximity;
	static UINT button;
	static UINT	pressure;
	BYTE *data;
	ULONG dataNumber = ((BLUETOOTH_GATT_VALUE_CHANGED_EVENT *)EventOutParameter)->CharacteristicValue->DataSize;

	data = ((BLUETOOTH_GATT_VALUE_CHANGED_EVENT *)EventOutParameter)->CharacteristicValue->Data;

	//-----------------
	// Proximity status
	//-----------------
	if ((data[0] & 0x10) != 0) proximity = 1;
	else proximity = 0;

	//------------------
	// Pen Button status
	//------------------
	TmpLow = (UINT)data[0];				// 1 x x Pr B2 B1 Tip 1
	button = (TmpLow >> 1) & 0x0007;	// 0 0 0 0  0  B2 B1  Tip
	
	//---------------
	// Pressure value
	//---------------
	TmpLow = (UINT)data[5];
	TmpHigh = (UINT)data[6];
	pressure = (TmpLow & 0x007f) | ((TmpHigh << 7) & 0x0380);

	//-------------------
	// X coordinate value
	//-------------------
	TmpLow = (UINT)data[1];
	TmpHigh = (UINT)data[2];
	x_coor = (TmpLow & 0x007F) | ((TmpHigh << 7) & 0x3F80);
	
	//-------------------
	// Y coordinate value
	//-------------------
	TmpLow = (UINT)data[3];
	TmpHigh = (UINT)data[4];
	y_coor = (TmpLow & 0x007F) | ((TmpHigh << 7) & 0x3F80);

	wParam = (pressure << 4) | (button << 1) | proximity;
	lParam = x_coor | y_coor << 16;

	::PostMessage(((CChildView *)Context)->m_hWnd, WM_PENPAPER_DATA_READY, wParam, lParam);

}


//================================================
// Common routine and variables for both method
//================================================
void CChildView::InitPenPaperDrawingArea()
{
	CDC* mainDC = GetDC();
	CPoint pp1, pp2;
	CRgn rgn;

	pp1.x = DRAW_AREA_LEFT;
	pp1.y = DRAW_AREA_TOP;
	pp2.x = DRAW_AREA_LEFT + 2 + DRAW_AREA_WIDTH;
	pp2.y = DRAW_AREA_TOP + 2 + DRAW_AREA_HEIGHT;

	/* Clear drawing area */
	mainDC->SelectStockObject(WHITE_PEN);
	rgn.CreateRectRgn(pp1.x, pp1.y, pp2.x, pp2.y);
	mainDC->PaintRgn(&rgn);
	rgn.DeleteObject();

	/* Draw frame of drawing area */
	mainDC->SelectStockObject(BLACK_PEN);
	mainDC->Rectangle(pp1.x, pp1.y, pp2.x, pp2.y);

	ReleaseDC(mainDC);

	b_redPointShowing = false;
}

void CChildView::ClearMessageArea()
{
	CDC* mainDC = GetDC();
	CPoint pp1, pp2;
	CRgn rgn;
	RECT rect;

	GetClientRect(&rect);
	rgn.CreateRectRgn(0, 160, MESSAGE_AREA_WIDTH - 1, rect.bottom - rect.top);
	mainDC->SelectStockObject(WHITE_PEN);
	mainDC->PaintRgn(&rgn);

	ReleaseDC(mainDC);
}

//------------------------------------
// Show data and draw pressure strokes
//------------------------------------
void CChildView::ShowPenPaperData(void)
{
	CDC* mainDC = GetDC();
	CString csString;
	CPoint pp1;
	CPen* oldPen;
	int tempROP2;
	int current_tipon;

	//---------------------------------
	// These are for draw the red-point 
	//---------------------------------
	static CPoint newPoint;
	static CPoint lastPoint;

	static PointF newPoint_gdip;
	static PointF lastPoint_gdip;

	static int last_button;
	static int last_tipon;

	Color color;
	Pen pen(Color::Black, 0.1f);

	CFont* CurFont;
	CFont NewFont;
	LOGFONT lf;

	current_tipon = button & 0x0001;

	mainDC->SetTextColor(COLOR_BLUE);

	pp1.x = 10;
	pp1.y = 160;

	csString.Format(L"Packages : %.8d", i_count);
	mainDC->TextOut(pp1.x, pp1.y, csString);
	pp1.y = pp1.y + 22;

	csString.Format(L"X : %.4d          ", x_coor);
	mainDC->TextOut(pp1.x, pp1.y, csString);
	csString.Format(L"Y : %.4d        ", y_coor);
	mainDC->TextOut(pp1.x + 90, pp1.y, csString);
	pp1.y = pp1.y + 22;

	csString.Format(L"Pressure : %.4d   ", pressure);
	mainDC->TextOut(pp1.x, pp1.y, csString);
	pp1.y = pp1.y + 22;

	if ((button & 0x0001) != 0) csString.Format(L"Tip -----> on  ");
	else csString.Format(L"Tip -----> off");
	mainDC->TextOut(pp1.x, pp1.y, csString);
	pp1.y = pp1.y + 22;

	csString.Format(L"Button : %d", button);
	mainDC->TextOut(pp1.x, pp1.y, csString);
	pp1.y = pp1.y + 44;

	if (b_Proximity == true) csString.Format(L"In proximity         ");
	else
	{
		csString.Format(L"Out of Proximity");
		mainDC->SetTextColor(COLOR_RED);
	}
	mainDC->TextOut(pp1.x, pp1.y, csString);

	/* Show enhanced Button status */
	if (last_button != button)
	{
		CurFont = mainDC->GetCurrentFont();
		CurFont->GetLogFont(&lf);
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Arial");
		lf.lfHeight = 27;
		lf.lfWidth = 15;
		lf.lfWeight = FW_EXTRABOLD;
		//lf.lfItalic = TRUE;
		NewFont.CreateFontIndirect(&lf);
		CurFont = mainDC->SelectObject(&NewFont);

		if (button != 0)
		{
			CString csTemp;
			if (button == 1) mainDC->SetTextColor(COLOR_BLUE);
			else if (button <= 3) mainDC->SetTextColor(COLOR_RED);
			else if (button <= 5) mainDC->SetTextColor(COLOR_GREEN);
			csString.Format(L"button: %d", button);
		}
		else csString.Format(L"            ");
		pp1.y = pp1.y + 44;
		mainDC->TextOut(pp1.x, pp1.y, csString);
		mainDC->SelectObject(CurFont);
	}

	//----------------------------------------
	// Process the red-point if the tip is off
	//----------------------------------------
	newPoint = CPoint((int)(DRAW_AREA_LEFT + x_coor * drawAreaScaleX + 2), (int)(DRAW_AREA_HEIGHT - y_coor * drawAreaScaleY + DRAW_AREA_TOP + 2));

	if ((last_tipon == 0) && (current_tipon == 0))	/* Pen Move Up*/
	{
		//-------------------
		// Draw the red-point
		//-------------------
		tempROP2 = mainDC->GetROP2();
		mainDC->SetROP2(R2_XORPEN);
		oldPen = mainDC->SelectObject(pCursorPen);

		//-----------------------------------------
		// Erase the previous red point if it exist
		//-----------------------------------------
		if (b_redPointShowing == true)
		{
			mainDC->MoveTo(lastPoint);
			mainDC->LineTo(lastPoint);
		}

		mainDC->MoveTo(newPoint);
		mainDC->LineTo(newPoint);

		mainDC->SelectObject(oldPen);
		mainDC->SetROP2(tempROP2);

		b_redPointShowing = true;
	}
	else if ((last_tipon == 0) && (current_tipon == 1))	/* Pen Down, first point of stroke */
	{
		//--------------------------------------
		// Erase red point at the first pen-down
		//--------------------------------------
		if (b_redPointShowing == true)
		{
			tempROP2 = mainDC->GetROP2();
			mainDC->SetROP2(R2_XORPEN);
			oldPen = mainDC->SelectObject(pCursorPen);

			mainDC->MoveTo(lastPoint);
			mainDC->LineTo(lastPoint);

			mainDC->SetROP2(tempROP2);
			mainDC->SelectObject(oldPen);

			b_redPointShowing = false;
		}
	}

	//------------------------------------------
	// Draw the pressure stroke if tip switch on
	//------------------------------------------
	newPoint_gdip = PointF((REAL)(PenPaperdrawAreaLeft + x_coor), (REAL)(PENPAPER_HEIGHT - y_coor + PenPaperdrawAreaTop));

	if (last_tipon == 1)
	{
		Graphics* graphics = Graphics::FromHDC(mainDC->m_hDC);
		graphics->SetPageUnit(UnitPixel);
		graphics->SetSmoothingMode(SmoothingModeHighQuality);
		graphics->SetPageScale(drawAreaScaleX);

		pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
		pen.SetLineJoin(LineJoinRound);

		if (last_button == 1) color.SetFromCOLORREF(COLOR_BLUE);
		else if (last_button == 3) color.SetFromCOLORREF(COLOR_RED);
		else if (last_button == 5) color.SetFromCOLORREF(COLOR_GREEN);
		else color.SetFromCOLORREF(COLOR_BLACK);

		pen.SetColor(color);

		pen.SetWidth((REAL)(pressure / (float)i_pressureDivider));
		graphics->DrawLine(&pen, PointF(lastPoint_gdip.X, lastPoint_gdip.Y), PointF(newPoint_gdip.X, newPoint_gdip.Y));

		graphics->ReleaseHDC(mainDC->m_hDC);
		delete(graphics);
	}

	lastPoint = newPoint;
	last_tipon = current_tipon;
	last_button = button;
	lastPoint_gdip = newPoint_gdip;
	ReleaseDC(mainDC);
}

