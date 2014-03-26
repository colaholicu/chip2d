// chip2d.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "chip2d.h"
#include "d2dwrap.h"
#include "emucore.h"

using namespace D2DW;

#define MAX_LOADSTRING 100

float pixelSizeX = 10.f;
float pixelSizeY = 10.f;

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

static Chip2d chip;
bool Save(Chip2d& chip, HWND hWindow);
bool Open(Chip2d& chip, HWND hWindow);
bool Load(Chip2d& chip, HWND hWindow);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;
    HWND hWindow;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CHIP2D, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHIP2D));
    hWindow = GetActiveWindow();

    if (!hWindow)
    {
        return FALSE;
    }           

    // init Direct2D
    createBasicD2D(hWindow);

    srand(GetTickCount());

    if (!chip.Initialize())
        return FALSE;

	// Main message loop:
    while (true)
    {
	    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	    {
            if (msg.message == WM_QUIT)
                break;

		    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		    {
			    TranslateMessage(&msg);
			    DispatchMessage(&msg);
		    }
	    } else {
            if (chip.started() && !chip.Cycle())
                break;
            
            chip.Draw();            
        }
    }

    destroyBasicD2D();

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHIP2D));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_CHIP2D);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

#define HEIGHT  320
#define WIDTH   640 

   int nXWindowPos = (GetSystemMetrics(SM_CXSCREEN) - WIDTH) / 2,
       nYWindowPos = (GetSystemMetrics(SM_CYSCREEN) - HEIGHT) / 2;

   RECT rc = {0};
   rc.bottom = HEIGHT;
   rc.right = WIDTH;

   AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

   int nWidth = rc.right - rc.left;
   int nHeight = rc.bottom - rc.top;

   hWnd = CreateWindowEx(
       WS_EX_CLIENTEDGE,
       szWindowClass, 
       szTitle, 
       WS_OVERLAPPEDWINDOW, 
       nXWindowPos, 
       nYWindowPos,
       nWidth, 
       nHeight, 
       NULL, 
       NULL, 
       hInstance, 
       NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);   

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
        case IDM_OPEN:
            Open(chip, hWnd);
            break;
        case IDM_SAVE:
            if (chip.started())
            {
                Save(chip, hWnd);
            }
            break;
        case IDM_LOAD:
            Load(chip, hWnd);
            break;
        case IDM_RESET:
            if (chip.started() && !chip.Reset()) 
            {
                MessageBox(NULL, "Chip2D failed to reset!", "Ooops!", MB_OK);
                DestroyWindow(hWnd);
            }
            break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
        //chip.Draw();
		EndPaint(hWnd, &ps);
		break;

    case WM_KEYDOWN:
        chip.keyDown(wParam);
        break;

    case WM_KEYUP:
        chip.keyUp(wParam);
        break;

    case WM_ERASEBKGND:
        return 1;

	case WM_DESTROY:
        D2DW::destroyBasicD2D();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool Save(Chip2d& chip, HWND hWindow)
{
    OPENFILENAME ofn;
    char szFileName[64] = {0};
    ZeroMemory(&ofn, sizeof(ofn));


    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = NULL;
    ofn.hwndOwner = hWindow;
    ofn.lpstrTitle = "Save slot";
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrFile = szFileName;
    ofn.lpstrFilter = "Save slots (*.sav)\0*.sav\0All files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_CREATEPROMPT;

    GetSaveFileName(&ofn);
    if (!szFileName)
        return false;

    if (!chip.Save(szFileName))
    {
        MessageBox(NULL, "Chip2D failed to save slot!", "Ooops!", MB_OK);
        return false;
    }

    return true;
}

bool Load(Chip2d& chip, HWND hWindow)
{

    OPENFILENAME ofn;
    char szFileName[64] = {0};
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = NULL;
    ofn.hwndOwner = hWindow;
    ofn.lpstrTitle = "Find save slot";
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrFile = szFileName;
    ofn.lpstrFilter = "Save slots (*.sav)\0*.sav\0All files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;

    GetOpenFileName(&ofn);
    if (!szFileName)
        return false;

    if (!chip.Load(szFileName))
    {
        MessageBox(NULL, "Chip2D failed to load save slot!", "Ooops!", MB_OK);
        return false;
    }

    return true;
}

bool Open(Chip2d& chip, HWND hWindow)
{
    OPENFILENAME ofn;
    char szFileName[64] = {0};
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = NULL;
    ofn.hwndOwner = hWindow;
    ofn.lpstrTitle = "Find ROM";
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrFile = szFileName;
    ofn.lpstrFilter = "Chip ROMs (*.c8)\0*.c8\0All files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER ;

    GetOpenFileName(&ofn);
    if (!szFileName)
        return false;

    if (!chip.Reset() || !chip.LoadRom(szFileName))
    {
        MessageBox(NULL, "Chip2D failed to load ROM!", "Ooops!", MB_OK);
        return false;
    }

    return true;
}