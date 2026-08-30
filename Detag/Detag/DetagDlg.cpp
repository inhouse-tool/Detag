// DetagDlg.cpp : implementation file
//

#include "pch.h"
#include "DetagApp.h"
#include "DetagDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define	AI_COPILOT	1	// Copilot             resides in https://copilot.microsoft.com/	
#define	AI_GEMINI	2	// Gemini web app      resides in https://gemini.google.com
#define	AI_GOOGLE	3	// Google AI Overviews resides in https://www.google.com

#define	FILE_HTML	1
#define	FILE_MD		2

CDetagDlg::CDetagDlg( CWnd* pParent )
	: CDialog( IDD_DETAG_DIALOG, pParent )
{
	m_hIcon = AfxGetApp()->LoadIcon( IDR_MAINFRAME );
}

BOOL
CDetagDlg::OnInitDialog( void )
{
	CDialog::OnInitDialog();

	SetIcon( m_hIcon, TRUE );
	SetIcon( m_hIcon, FALSE );

	((CButton*)GetDlgItem( IDC_RADIO_MD ))->SetCheck( BST_CHECKED );

	CMenu*	pSysMenu = GetSystemMenu( FALSE );
	if	( pSysMenu ){
		pSysMenu->InsertMenu( SC_CLOSE, MF_BYCOMMAND | MF_STRING, ID_APP_ABOUT, L"About this app..." );
		pSysMenu->InsertMenu( SC_CLOSE, MF_BYCOMMAND | MF_STRING, ID_HELP,      L"Visit web site of this app\tF1" );
		pSysMenu->InsertMenu( SC_CLOSE, MF_SEPARATOR, 0, L"" );
	}

	return	TRUE;
}

BEGIN_MESSAGE_MAP( CDetagDlg, CDialog )
	ON_WM_DROPFILES()
	ON_WM_SYSCOMMAND()
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

void
CDetagDlg::OnDropFiles( HDROP hDrop )
{
	GetDlgItem( IDC_STATIC     )->EnableWindow( FALSE );
	GetDlgItem( IDC_RADIO_MD   )->EnableWindow( FALSE );
	GetDlgItem( IDC_RADIO_HTML )->EnableWindow( FALSE );

	UINT	nFile =
	DragQueryFile( hDrop, -1, NULL, 0 );

	for	( UINT iFile = 0; iFile < nFile; iFile++ ){
		TCHAR	szFile[MAX_PATH];
		DragQueryFile( hDrop, iFile, szFile, _countof( szFile ) );
		CString	strFile( szFile );
		if	( !strFile.Right( 5 ).CompareNoCase( L".html" ) ||
			  !strFile.Right( 4 ).CompareNoCase( L".htm"  )    )
			DetagHTML( strFile );
	}

	GetDlgItem( IDC_STATIC     )->EnableWindow( TRUE );
	GetDlgItem( IDC_RADIO_MD   )->EnableWindow( TRUE );
	GetDlgItem( IDC_RADIO_HTML )->EnableWindow( TRUE );
}

BOOL
CDetagDlg::OnHelpInfo( HELPINFO* pHelpInfo )
{
	OnWeb();

	return	TRUE;
}

void
CDetagDlg::OnSysCommand( UINT nID, LPARAM lParam )
{
	WORD	wID = nID & 0xFFFF;

	if	( wID == ID_HELP )
		OnWeb();

	else if	( wID == ID_APP_ABOUT )
		OnAppAbout();
	else
		CDialog::OnSysCommand( nID, lParam );
}

void
CDetagDlg::OnWeb( void )
{
	CString	strURL;

	strURL.Insert( 0, L"http://github.com/inhouse-tool/Detag" );
	ShellExecute( NULL, _T("open"), strURL, NULL, NULL, SW_SHOWNORMAL );
}

#pragma comment( lib, "version.lib" )

void
CDetagDlg::OnAppAbout( void )
{
	CString	strCaption = AfxGetApp()->m_pszAppName;
	strCaption.Insert( 0, _T("About ") );
	CString	strText;

	{
		unsigned	uLen;
		TCHAR		achPath[_MAX_PATH];
		GetModuleFileName( NULL, achPath, _countof( achPath ) );

		DWORD	dwHandle;
		DWORD	dwLen = GetFileVersionInfoSize( achPath, &dwHandle );
		char*	pchVerInfo = new char[dwLen];

		GetFileVersionInfo( achPath, 0, dwLen, (LPVOID)pchVerInfo );
		VS_FIXEDFILEINFO*	info;
		VerQueryValue( pchVerInfo, _T("\\"), (LPVOID*)&info, &uLen );

		CString	str, strSubBlock;
		LPVOID	pValue;

		VerQueryValue( pchVerInfo, _T("\\VarFileInfo\\Translation"), &pValue, &uLen );
		strSubBlock.Format( _T("\\StringFileInfo\\%04x%04x\\"), *(int*)pValue & 0xffff, *(int*)pValue>>16 );

		str = strSubBlock + _T("ProductName");
		VerQueryValue( pchVerInfo, str, &pValue, &uLen );
		strText = (LPCTSTR)pValue;
		strText += _T("\n");

		str = strSubBlock + _T("FileDescription");
		VerQueryValue( pchVerInfo, str, &pValue, &uLen );
		strText += (LPCTSTR)pValue;

		str.Format( _T("\n\nProgram Version %d.%d.%d"),
				( info->dwProductVersionMS >> 16 ),
				( info->dwProductVersionMS &  0xffff ),
				( info->dwProductVersionLS >> 16 ) );
		strText += str;
		strText += _T("\n");

		str = strSubBlock + _T("LegalCopyright");
		VerQueryValue( pchVerInfo, str, &pValue, &uLen );
		strText += (LPCTSTR)(char*)pValue;

		delete[]	pchVerInfo;
	}

	MSGBOXPARAMS	mbp;

	mbp.cbSize = sizeof( MSGBOXPARAMSA );
	mbp.hwndOwner    = m_hWnd;
	mbp.hInstance    = AfxGetApp()->m_hInstance;
	mbp.lpszText     = strText.GetBuffer();
	mbp.lpszCaption  = strCaption.GetBuffer();
	mbp.dwStyle      = MB_OK | MB_USERICON;
	mbp.lpszIcon     = MAKEINTRESOURCE( IDR_MAINFRAME );
	mbp.dwContextHelpId    = 0;
	mbp.lpfnMsgBoxCallback = NULL;
	mbp.dwLanguageId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );

	MessageBoxIndirect( &mbp );
}

#include "Detag.h"

void
CDetagDlg::DetagHTML( CString strFile )
{
	// Load text from the given file.

	CString	strHTML = LoadText( strFile );

	// Extract the minimal HTML text.

	CDetag	detag;
	CString	strOut = detag.Detag( strHTML );
	if	( strOut.IsEmpty() )
		return;

	// Make a target file name and adjust the image.

	int	x = strFile.ReverseFind( '.' );
	CString	strExt = strFile.Mid( x );
	strFile = strFile.Left( x );

	// Select the target file type.

	int	iFile =
		( ((CButton*)GetDlgItem( IDC_RADIO_MD )  )->GetCheck() == BST_CHECKED )?	FILE_MD:
		( ((CButton*)GetDlgItem( IDC_RADIO_HTML ))->GetCheck() == BST_CHECKED )?	FILE_HTML:
												0;
	if	( iFile == FILE_MD ){
		strFile += L".md";
	}
	else if	( iFile == FILE_HTML ){
		strFile += ( strExt.CompareNoCase( L".htm" ) == 0 )? L".html": L".htm";

		LPCTSTR	pszHTMLheader = L""
			"<!DOCTYPE html>\r\n"
			"<html>\r\n"
			"<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\r\n"
			"<style>\r\n"
			"\tbody{\r\n"
			"\t\tfont-family: sans-serif;\r\n"
			"\t\tcolor: #0d1117;\tbackground-color: #e6edf3;\r\n"
			"\t\tmargin-left: 2%;\tmargin-right: 2%;\r\n"
			"\t}\r\n"
			"\ttable{\r\n"
			"\t\tborder-collapse: collapse;\r\n"
			"\t}\r\n"
			"\tth, td{"
			"\t\tborder: 1px solid #7f7f7f;\r\n"
			"\t\tpadding: 8px;\r\n"
			"\t}\r\n"
			"\r\n"
			"@media ( prefers-color-scheme: dark ){\r\n"
			"\tbody {\r\n"
			"\t\tcolor: #e6edf3;\tbackground-color: #0d1117;\r\n"
			"\t}\r\n"
			"</style>\r\n"
			"</head>\r\n"
			"<body link=\"#4088e7\" vlink=\"#4088e7\">\r\n\r\n",

			pszHTMLfooter = L""
			"\r\n"
			"</body>\r\n"
			"</html>\r\n";

		strOut.Insert( 0, pszHTMLheader );
		strOut += pszHTMLfooter;
	}
	else
		strOut.Empty();

	// Save the image to the target file.

	if	( !strOut.IsEmpty() )
		SaveText( strFile, strOut );
}

#define	CP_SHIFT_JIS	  932

CString
CDetagDlg::LoadText( CString strFile )
{
	CString	strLines;
	enum	Encode{
		unknown,
		ASCII,
		ShiftJIS,
		UTF8,
		UTF16BE,
		UTF16LE,
		UTF32BE,
		UTF32LE
	}	eEncode;
	DWORD	cbBOM = 0;
	DWORD	dwBOM = 0;

	CFile	f;
	if	( f.Open( strFile, CFile::modeRead | CFile::shareExclusive ) ){
		DWORD	cbData = (DWORD)f.GetLength();
		BYTE*	pbData = new BYTE[cbData+2];

		f.Read( pbData, cbData );
		pbData[cbData+0] = '\0';
		pbData[cbData+1] = '\0';

		BYTE*	pb = pbData;

		// If the data is empty, encoding is unknown.

		if	( cbData == 0 )
			eEncode = unknown;

		// If the BOM exists, encoding is written in BOM.

		else if	( pb[0] == 0xef && pb[1] == 0xbb && pb[2] == 0xbf ){
			cbBOM = 3;
			memcpy( &dwBOM, pb, cbBOM );
			eEncode = UTF8;
		}
		else if	( pb[0] == 0xff && pb[1] == 0xfe ){
			cbBOM = 2;
			memcpy( &dwBOM, pb, cbBOM );
			eEncode = UTF16LE;
		}
		else if	( pb[0] == 0xfe && pb[1] == 0xff ){
			cbBOM = 2;
			memcpy( &dwBOM, pb, cbBOM );
			eEncode = UTF16BE;
		}
		else if	( pb[0] == 0x00 && pb[1] == 0x00 && pb[2] == 0xfe && pb[3] == 0xff ){
			cbBOM = 4;
			memcpy( &dwBOM, pb, cbBOM );
			eEncode = UTF32BE;
		}
		else if	( pb[0] == 0xff && pb[1] == 0xfe && pb[2] == 0x00 && pb[3] == 0x00 ){
			cbBOM = 4;
			memcpy( &dwBOM, pb, cbBOM );
			eEncode = UTF32LE;
		}
		// If there's no BOM, judge encoding from some data.

		else{
			Encode	aeEncode[16] = {};
			int	nEncode = 0;

			for	( QWORD cb = cbData; cb > 0; ){
				if	( *pb < 0x7f ){
					cb--;
					pb++;
				}
				else if	( pb[0] >= 0xc2 && pb[0] <= 0xdf &&
					  pb[1] >= 0x80 && pb[1] <= 0xbf ){
					aeEncode[nEncode++] = UTF8;	// 11bit code
					cb -= 2;
					pb += 2;
				}
				else if	( pb[0] >= 0xe0 && pb[0] <= 0xef &&
					  pb[1] >= 0x80 && pb[1] <= 0xbf &&
					  pb[2] >= 0x80 && pb[2] <= 0xbf ){
					aeEncode[nEncode++] = UTF8;	// 16bit code
					cb -= 3;
					pb += 3;
				}
				else if	( pb[0] >= 0xf0 && pb[0] <= 0xf4 &&
					  pb[1] >= 0x80 && pb[1] <= 0xbf &&
					  pb[2] >= 0x80 && pb[2] <= 0xbf &&
					  pb[3] >= 0x80 && pb[3] <= 0xbf ){
					aeEncode[nEncode++] = UTF8;	// 21bit code
					cb -= 4;
					pb += 4;
				}
				else if	( ( ( pb[0] >= 0x81 && pb[0] <= 0x9f ) ||
					    ( pb[0] >= 0xe0 && pb[0] <= 0xef )    ) &&
					  ( ( pb[1] >= 0x40 && pb[1] <= 0x7e ) ||
					    ( pb[0] >= 0x80 && pb[0] <= 0xfc )    )    ){
					aeEncode[nEncode++] = ShiftJIS;
					cb -= 2;
					pb += 2;
				}
				else{
					aeEncode[nEncode++] = unknown;
					break;
				}
				if	( nEncode >= _countof( aeEncode ) )
					break;
			}
			{
				int	i;
				for	( i = 1; i < nEncode; i++ )
					if	( aeEncode[i] != aeEncode[0] )
						break;	
				eEncode =	( nEncode == 0 )?	ASCII:
						( i >= nEncode )?	aeEncode[0]:
									unknown;
			}
		}

		CHAR*	pbText = (CHAR*)( pbData + cbBOM );

		if	( eEncode == ASCII ){
			int	cwch =
			MultiByteToWideChar( CP_ACP, 0, pbText, -1, NULL, 0 );
			WCHAR*	pwch = new WCHAR[cwch+1];
			MultiByteToWideChar( CP_ACP, 0, pbText, -1, pwch, cwch );
			pwch[cwch] = '\0';
			strLines = pwch;
			delete	[]pwch;
		}
		else if	( eEncode == ShiftJIS ){
			int	cwch =
			MultiByteToWideChar( CP_SHIFT_JIS, 0, pbText, -1, NULL, 0 );
			WCHAR*	pwch = new WCHAR[cwch+1];
			MultiByteToWideChar( CP_SHIFT_JIS, 0, pbText, -1, pwch, cwch );
			pwch[cwch] = '\0';
			strLines = pwch;
			delete	[]pwch;
		}
		else if	( eEncode == UTF8 ){
			int	cwch =
			MultiByteToWideChar( CP_UTF8, 0, pbText, -1, NULL, 0 );
			WCHAR*	pwch = new WCHAR[cwch+1];
			MultiByteToWideChar( CP_UTF8, 0, pbText, -1, pwch, cwch );
			pwch[cwch] = '\0';
			strLines = pwch;
			delete	[]pwch;
		}
		else if	( eEncode == UTF16LE )
			strLines = (WCHAR*)pbText;

		delete []pbData;
	}

	// Ensure "\r\n" not "\n".

	int	x = 0;
	for	( ;; ){
		int	i = strLines.Find( '\n', x );
		if	( i < 0 )
			break;
		if	( i == 0 || strLines[i-1] != '\r' )
			strLines.Insert( i++, L"\r" );
		x = i+1;
	}

	return	strLines;
}

void
CDetagDlg::SaveText( CString strFile, CString strContents )
{
	CFile	f;
	if	( f.Open( strFile, CFile::modeWrite | CFile::modeCreate | CFile::shareExclusive ) ){

		// Convert UTF-16 to UTF-8.

		WCHAR*	pchIn = (WCHAR*)strContents.GetString();
		int	cchOut =
		::WideCharToMultiByte( CP_UTF8, 0, pchIn, -1, NULL,        0, NULL, NULL );
		CHAR*	pchOut = new CHAR[cchOut];
		::WideCharToMultiByte( CP_UTF8, 0, pchIn, -1, pchOut, cchOut, NULL, NULL );

		DWORD	dwBOM = 0xbfbbef;
		f.Write( &dwBOM, 3 );
		f.Write( pchOut, cchOut-1 );
		delete []pchOut;
	}
}
