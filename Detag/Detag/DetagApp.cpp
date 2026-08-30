// DetagApp.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "DetagApp.h"
#include "DetagDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CDetagApp theApp;

BOOL
CDetagApp::InitInstance( void )
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof( InitCtrls );
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx( &InitCtrls );

	CWinApp::InitInstance();

	CDetagDlg	dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	return	FALSE;
}

