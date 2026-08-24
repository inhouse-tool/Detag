// Detag.h : header file
//

#pragma once

class CDetag
{
public:
		CString	Detag( CString strHTML );

protected:
		CString	DetagAIs( CString strHTML );

		void	DetagCopilot( CString strHTML, CString strElement, CString strTag, CString& strOut, void* pvArgs );
		void	DetagGemini(  CString strHTML, CString strElement, CString strTag, CString& strOut, void* pvArgs );
		void	DetagGoogle(  CString strHTML, CString strElement, CString strTag, CString& strOut, void* pvArgs );
		void	DetagCommon(  CString strHTML, CString strElement, CString strTag, CString& strOut, void* pvArgs );

		void	TrimCopilot( CString& strOut, bool bLast );
		void	TrimGoogle( CString& strOut, CString strPart );
		void	ListGoogle( CString& strOut, CString strPart );
		void	TrimQandA( CString& strOut );
		void	TrimQuery( CString& strQuery );

		int	SkipBranch( CString strHTML, int iIndex, CString strElement );
		int	IsTagToDelete( CString strHTML, int iIndex, CString strElement );
		bool	IsTagToKeep( CString strElement );
		int	SeekTag( CString strHTML, int iIndex, LPCTSTR pszTag );

		void	OptimizeHTML( CString& strOut );

		int	ReverseFind( LPCTSTR pszBase,  LPCTSTR pszPattern, int iIndex = -1 );
};
