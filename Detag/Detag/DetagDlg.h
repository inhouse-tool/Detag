// DetagDlg.h : header file
//

#pragma once

class CDetagDlg : public CDialog
{
public:
	CDetagDlg( CWnd* pParent = nullptr );

protected:
		HICON	m_hIcon;

	virtual	BOOL	OnInitDialog();

	afx_msg	void	OnDropFiles( HDROP hDrop );
	afx_msg	BOOL	OnHelpInfo( HELPINFO* pHelpInfo );
	afx_msg	void	OnSysCommand( UINT nID, LPARAM lParam );
	DECLARE_MESSAGE_MAP()

		void	OnWeb( void );
		void	OnAppAbout( void );

		void	DetagHTML( CString strFile );

		CString	LoadText( CString strFile );
		void	SaveText( CString strFile, CString strContents );
};
