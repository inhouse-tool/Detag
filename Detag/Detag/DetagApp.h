// DetagApp.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"

class CDetagApp : public CWinApp
{
protected:
	virtual	BOOL	InitInstance( void );
};

extern	CDetagApp	theApp;
