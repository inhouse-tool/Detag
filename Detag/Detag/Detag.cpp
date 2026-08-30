// Detag.cpp : implementation file
//

#include "pch.h"
#include "Detag.h"

///////////////////////////////////////////////////////////////////////////////////////
// Public Functions

CString
CDetag::Detag( CString strHTML )
{
	// Make a converted image from the given file.

	CString	strOut = DetagAIs( strHTML );
	
	OptimizeHTML( strOut );

	return	strOut;
}

///////////////////////////////////////////////////////////////////////////////////////
// Protected Functions

typedef	struct{
	bool	bOut;		// Output open/close
	bool	bAnswered;	// Already answerd
	int	x;		// Where '<'
	int	xClose;		// Where '>'
	int	xNext;		// Where to start next seek
	int	xLast;		// Where to close an answer
	int	nOpenDiv;	// Nest count of open <div>
}	Args;

CString
CDetag::DetagAIs( CString strHTML )
{
	CString	strOut;

	// Determine which AI the input is from.

	enum	{ none = 0, copilot, gemini, google }
		eAI = none;
	{
		int	x  = strHTML.Find( L"<body" );
		if	( x < 0 )
			return	strOut;

		CString	strHead = strHTML.Left( x );
		strHTML.Delete( 0, x );

		int	xCopilot = strHead.Find( L"https://copilot.microsoft.com" );
		int	xGemini  = strHead.Find( L"https://gemini.google.com" );
		int	xGoogle  = strHead.Find( L"https://www.google.com" );

		if	( xCopilot < 0 )
			xCopilot = INT_MAX;
		if	( xGemini  < 0 )
			xGemini  = INT_MAX;
		if	( xGoogle  < 0 )
			xGoogle  = INT_MAX;

		x = min( min( xCopilot, xGemini ), xGoogle );
		if	( x != INT_MAX )
			eAI =	( x == xCopilot )?	copilot:
				( x == xGemini )?	gemini:
				( x == xGoogle )?	google:
							none;
		if	( eAI == none )
			return	strOut;
	}

	Args	args = { 0 };

	// Do for all tags.

	while	( args.xNext >= 0 ){

		// As far as the next is found...

		args.x = strHTML.Find( L"<", args.xNext );
		if	( args.x < 0 )
			break;

		// Output the text between tags.

		if	( args.x > args.xNext )
			if	( args.bOut ){
				CString	strText = strHTML.Mid( args.xNext, args.x-args.xNext );
				strOut += strText;
			}

		// Get the tag name.

		CString	strElement, strTag;

		args.xClose = strHTML.Find( L">", args.x );
		if	( args.xClose < 0 ){
			TRACE( L"unclosed tag '%s'\n", strHTML.Mid( args.x, 16 ) );
			break;
		}
		else{
			int	xSep = strHTML.Find( L" ", args.x );
			if	( args.xClose < xSep )
				xSep = args.xClose;

			strElement = strHTML.Mid( args.x+1, xSep-args.x-1 );
			if	( strElement.Left( 3 ) == L"!--" ){
				args.xNext = SeekTag( strHTML, args.x, L"-->" );
				continue;
			}

			int	n = args.xClose-args.x+1;
			strTag = strHTML.Mid( args.x, n );
		}

		// Unnecessary tags: Just skip to the next.
		{
			args.xNext = -1;

			// Skip tags to delete.

			int	xAfter = IsTagToDelete( strHTML, args.x, strElement );
			if	( xAfter >= 0 )
				args.xNext = xAfter;

			// Gemini has many custom tags. Scrape them.

			else if	( eAI == gemini )
				if	( !IsTagToKeep( strElement ) )
					args.xNext = SeekTag( strHTML, args.x, L">" );

			if	( args.xNext >= 0 )
				continue;
		}

		// Copilot specific tags:

		if	( eAI == copilot )
			DetagCopilot( strHTML, strElement, strTag, strOut, &args );

		// Gemini specific tags:

		else if	( eAI == gemini )
			DetagGemini( strHTML, strElement, strTag, strOut, &args );

		// Google specific tags:

		else if	( eAI == google )
			DetagGoogle( strHTML, strElement, strTag, strOut, &args );

		// Common tags:

		if	( args.xNext < 0 && (
			  eAI == copilot ||
			  eAI == gemini  ||
			  eAI == google     ) )
			DetagCommon( strHTML, strElement, strTag, strOut, &args );

		// Add a line break ( just for readability of the output text ).

		if	( args.bOut && strElement != L"span" && strElement != L"/span" && strElement != L"code" )
			if	( strOut.Right( 2 ) != L"\r\n" )
				strOut += L"\r\n";
	}

	// Finish.

	if	( eAI == copilot )
		TrimCopilot( strOut, true );
	else if	( eAI == google )
		TrimGoogle( strOut, strHTML.Mid( args.xLast, args.x-args.xLast ) );

	return	strOut;
}

void
CDetag::DetagCopilot( CString strHTML, CString strElement, CString strTag, CString& strOut, void* pvArgs )
{
	Args&	args = *(Args*)pvArgs;

	// When entered into the <main>: Open the output;

	if	( strElement == L"main" ){
		args.bOut = true;
		args.xNext = SeekTag( strHTML, args.x, L">" );
	}

	// End of the contetnts: Close the output.

	else if	( strElement == L"/main" )
		args.bOut = false;

	// <h5>: Trim the the last answers before.

	else if	( strElement == L"h5" ){
		args.xNext = SeekTag( strHTML, args.x, L">" );
		TrimCopilot( strOut, false );
	}

	// <div ...>: Delete the tag.

	else if	( strElement == L"div" ){
		args.xNext = SeekTag( strHTML, args.x, L">" );

		// While a <div> is open: Nest one more.

		if	( args.nOpenDiv )
			args.nOpenDiv++;
	}

	// <span ...>:  Delete the tag.

	else if	( strElement == L"span" ){

		// <span class="block" ...: Delete to </span>

		if	( strTag.Find( L"class=\"block\"" ) >= 0 )
			args.xNext = SeekTag( strHTML, args.x, L"</span>" );

		// Other <span: Leave them.

		else
			args.xNext = SeekTag( strHTML, args.x, L">" );
	}
}

void
CDetag::DetagGemini( CString strHTML, CString strElement, CString strTag, CString& strOut, void* pvArgs )
{
	Args&	args = *(Args*)pvArgs;

	// When entered into the <chat-window>: Open the output;

	if	( strElement == L"chat-window" ){
		args.bOut = true;
		args.xNext = SeekTag( strHTML, args.x, L">" );
	}

	// User profile at the bottom of the session: Close the output.

	else if	( strElement == L"user-profile-picture" ){
		strOut += L"<hr>\r\n";
		args.bOut = false;
	}

	// Title: Remove it.

	else if	( strElement == L"h1" )
		args.xNext = SeekTag( strHTML, args.x, L"</h1>" );

	// Gemini's answer: Show the first paragraph as an answer.

	else if	( strElement == L"h2" ||	//OLD:
		  strElement == L"h6" ){	//NEW:
		args.xNext = strHTML.Find( L"<p", args.x );

		strOut += L"<h6>\r\nGemini said\r\n</h6>\r\n";
		TrimQandA( strOut );
	}

	// User's question: Take a query text behind "<span>Your prompt</span>".

	else if	( strElement == L"h5" ){
		args.xNext = strHTML.Find( L"<h6", args.x );

		CString	strQuery;
		CString	strH5 = strHTML.Mid( args.x, args.xNext-args.x );
		for	( int x0 = 0; x0 >= 0; ){
			x0 = strH5.Find( L"<p", x0 );
			if	( x0 < 0 )
				break;
			int	x1 = strH5.Find( '>', x0 );
			int	x2 = strH5.Find( '<', ++x1 );
			CString	strP = strH5.Mid( x1, x2-x1 );
			strQuery += strP + L"\r\n";
			x0 = x2;
		}
		strOut += L"<hr>\r\n<h5>\r\nYou said\r\n</h5>\r\n";
		strOut += strQuery;
	}

	// <div ...>: Delete the tag.

	else if	( strElement == L"div" ){

		args.xNext = SeekTag( strHTML, args.x, L">" );

		// While a <div> is open: Nest one more.

		if	( args.nOpenDiv )
			args.nOpenDiv++;

		//OLD: Heading: Break line and set larger.

		else if	( strTag.Find( L"role=\"heading\"" ) >= 0 ){

			// User's prompt ( for Gemini ):

			if	( strTag.Find( L"aria-level=\"2\"" ) >= 0 ){
				args.xNext = SkipBranch( strHTML, args.x, L"div" );
				int	x1 = strHTML.Find( L"<p", args.x );
				if	( x1 < 0 )
					return;

				CString	strQuery;
				CString	strPrompt = strHTML.Mid( x1, args.xNext-x1 );
				for	( int i = 0;; ){
					int	i1   = strPrompt.Find( L"<p", i );
					if	( i1 < 0 )
						break;
					int	i2   = strPrompt.Find( L">", i1 );
					int	i3   = strPrompt.Find( L"<", ++i2 );
					CString	strP = strPrompt.Mid( i2, i3-i2 );
					strP.Trim();
					strQuery += strP + L"\r\n";
					i = ++i3;
				}
				strQuery.TrimRight();
				strOut += L"<hr>\r\n<h5>\r\nYou said\r\n</h5>\r\n";
				strOut += strQuery;
			}

			// Other ARIA Levels are not used in Gemini web app ( for now ).
		}
	}

	// <span ...>:  Delete the tag.

	else if	( strElement == L"span" ){
		args.xNext = SeekTag( strHTML, args.x, L">" );
	}
}

void
CDetag::DetagGoogle( CString strHTML, CString strElement, CString strTag, CString& strOut, void* pvArgs )
{
	Args&	args = *(Args*)pvArgs;

	// <h2> a mark for uploaded data: Remove whole <h2>.

	if	( strElement == L"h2" ){
		args.xNext = SkipBranch( strHTML, args.x, L"h2" );
		TrimGoogle( strOut, strHTML.Mid( args.xLast, args.x-args.xLast ) );
		args.xLast = args.x;

		if	( strOut.Right( 6 ) != L"<hr>\r\n" )
			strOut += L"<hr>\r\n";
		strOut += L"<h5>\r\nYou said\r\n</h5>\r\n";

		args.bAnswered = false;
		if	( !args.bOut )
			args.bOut = true;
	}

	// <h3> a decolated text of query from the user: Remove whole <h3>.

	else if	( strElement == L"h3" ){
		args.xNext = SkipBranch( strHTML, args.x, L"h3" );
	}

	// <div ...>: Delete the tag.

	else if	( strElement == L"div" ){

		// Hidden style: Remove whole <div>.

		if	( strTag.Find( L"style=\"display: none\"" ) >= 0 )
			args.xNext = SkipBranch( strHTML, args.x, L"div" );

		// Share public link: Remove whole <div>.

		else if	( strTag.Find( L"role=\"dialog\"" ) >= 0 )
			args.xNext = SkipBranch( strHTML, args.x, L"div" );

		// Button for interaction: Remove whole <div>.

		else if	( strTag.Find( L"role=\"button\"" ) >= 0 )
			args.xNext = SkipBranch( strHTML, args.x, L"div" );

		// Hidden Accessible Rich Internet Applications: Remove whole <div>.

		else if	( strTag.Find( L"aria-hidden=\"true\"" ) >= 0 )
			args.xNext = SkipBranch( strHTML, args.x, L"div" );

		else
			args.xNext = SeekTag( strHTML, args.x, L">" );

		// While a <div> is open: Nest one more.

		if	( args.nOpenDiv )
			args.nOpenDiv++;

		// Heading: Break line and set larger.

		else if	( strTag.Find( L"role=\"heading\"" ) >= 0 ){

			// Unnumbered heading ( for Google ):

			if	( strTag.Find( L"aria-level=\"3\"" ) >= 0 ){
				if	( !strOut.IsEmpty() )
					strOut += L"\r\n<p>\r\n";
				if	( args.bOut ){
					strOut += L"<div style=\"font-size: 1.3rem;\">\r\n";
					args.nOpenDiv++;
				}
			}

			// Numbered heading ( for Google ):

			else if	( strTag.Find( L"aria-level=\"4\"" ) >= 0 ){
				if	( args.bOut )
					strOut += L"<p>";
			}

			// Other ARIA Levels in headings are not in Google AI Overviews ( for now ).
		}

		// AI Mode Container: Show as an answer.

		else if	( strTag.Find( L"data-subtree=\"aimc\"" ) >= 0 ){
			if	( !args.bAnswered ){
				strOut += L"<h6>\r\nGoogle said\r\n</h6>\r\n";
				args.bAnswered = true;
			}
		}
	}

	// <span ...>:  Delete the tag.

	else if	( strElement == L"span" ){

		args.xNext = SeekTag( strHTML, args.x, L">" );

		// <span> of heading: Show as a question.

		if	( strTag.Find( L"role=\"heading\"" ) >= 0 ){
			if	( !args.bOut )
				args.bOut = true;

			// User's question: Trim the the last answers before.

			if	( strTag.Find( L"aria-level=\"1\"" ) < 0 ){
				TrimGoogle( strOut, strHTML.Mid( args.xLast, args.x-args.xLast ) );
				args.xLast = args.x;
				if	( strOut.Right( 6 ) != L"<hr>\r\n" )
					strOut += L"<hr>\r\n";
				strOut += L"<h5>\r\nYou said\r\n</h5>\r\n";
				args.bAnswered = false;
			}
		}

		//OLD: AI Message HeadLine: Show as an answer.

		if	( strTag.Find( L"data-key=\"aimhl\"" ) >= 0 ||
			  strTag.Find( L"data-subtree=\"aimfl\"" ) >= 0 ){
			if	( !args.bAnswered ){
				strOut += L"<h6>\r\nGoogle said\r\n</h6>\r\n";
				args.bAnswered = true;
			}
		}

		// Accessible Rich Internet Applications are hidden: Skip whole <span>

		if	( strTag.Find( L"aria-hidden=\"true\"" ) >= 0 ){

			// Hidden Non-Break Space: Take it for a mark to insert a little space.

			int	x2 =  SeekTag( strHTML, args.xNext, L"<" );
			CString	strNext = strHTML.Mid( args.xNext, x2-args.xNext-1 );
			if	( strNext == L"&nbsp;" )
				if	( args.bOut )
					strOut += L"<p></p>\r\n";

			args.xNext = SkipBranch( strHTML, args.x, L"span" );
		}
	}
}

void
CDetag::DetagCommon( CString strHTML, CString strElement, CString strTag, CString& strOut, void* pvArgs )
{
	Args&	args = *(Args*)pvArgs;

	// Enquete text at the bottom: Close the output.

	if	( strElement == L"textarea" )
		args.bOut = false;

	// Do not add line breaks in <code ...> to </code>.

	else if	( strElement == L"code" ){
		int	xAfter = SeekTag( strHTML, args.x, L"</code>" );
		if	( xAfter < 0 )
			return;

		// Adjust the heading text on the <pre>\r\n<code>. ( for language name like 'python', 'javascript', 'html'... )
		{
			int	i = strOut.GetLength();
			for	( i -= 3; strOut[i] != '\r'; i-- )
				;
			CString	strLast = strOut.Mid( i );
			if	( strLast == L"\r\n<pre>\r\n" ){

				// Adjust size of heading text on the <pre><code>.

				for	( i -= 1; strOut[i] != '\r'; i-- )
					;
				int	iLast = i;
				if	( strOut.Mid( i-7, 7 ) == L"<p></p>" ){
					i -= 7;
					strOut.Delete( i, 7 );
				}
				if	( strOut.Mid( i-4, 4 ) == L"</p>" ){
					i -= 4;
					strOut.Delete( i, 4 );
				}
				strOut.Insert( i, L"\r\n<br><sub>" );
				for	( i += 2; strOut[i] != '\n'; i++ )
					;
				for	( i += 2; strOut[i] != '\n'; i++ )
					;
				strOut.Insert( ++i, L"</sub>\r\n\r\n" );
				// "\r\n\r\n" is to make a space above <pre>.
				// Because .md can not take <pre> without spaces above it.

				// Remove a little space on the <pre><code>.

				for	( i = iLast-3; strOut[i] != '\n'; i-- )
					;
				i++;
				strLast = strOut.Mid( i );
				if	( strLast.Find( L"<p></p>\r\n" ) == 0 )
					strOut.Delete( i, 9 );
				else if	( strLast.Find( L"</p>\r\n" ) == 0 )
					strOut.Delete( i, 6 );
			}
		}

		strTag = strHTML.Mid( args.x, xAfter-args.x );

		// Remove a comment in <code>.
		{
			int	i1 = strTag.Find( L"<!--" );
			if	( i1 > 0 ){
				int	i2 = SeekTag( strTag, i1, L"-->" );
				if	( i2 > 0 )
					strTag.Delete( i1, i2-i1 );
			}
		}

		// Remove arguments im <code>.
		{
			int	i1 = strTag.Find( L" " );
			int	i2 = strTag.Find( L">" );
			if	( i1 > 0 && i2 > 0 )
				strTag.Delete( i1, i2-i1 );
		}

		// Remove <span>s in <code>.
		for	( ;; ){
			int	i1 = strTag.Find( L"<span" );
			if	( i1 < 0 )
				break;

			int	i2 = strTag.Find( L">", i1 );
			if	( i2 < 0 )
				break;

			strTag.Delete( i1, i2-i1+1 );
		}
		for	( ;; ){
			int	i1 = strTag.Find( L"</span>" );
			if	( i1 < 0 )
				break;

			int	i2 = strTag.Find( L">", i1 );
			if	( i2 < 0 )
				break;

			strTag.Delete( i1, i2-i1+1 );
		}

		strOut += strTag;
		args.xNext = xAfter;
	}

	// </div>: Delete the tag.

	else if	( strElement == L"/div" ){
		args.xNext = SeekTag( strHTML, args.x, L">" );

		// <div> is open: Close it.

		if	( args.nOpenDiv )
			if	( !--args.nOpenDiv )
				strOut += L"</div><p><p>\r\n";
	}

	// </span>: Delete the tag.

	else if	( strElement == L"/span" ){
		args.xNext = SeekTag( strHTML, args.x, L">" );
	}

	// Other tags:

	else{
		if	( args.bOut ){
			// <a>: Simplify it.

			if	( strElement == L"a" ){
				int	i1 = strTag.Find( L"href=" );
				i1 += 6;
				int	i2 = strTag.Find( '"', i1 );
				CString	strURL = strTag.Mid( i1, i2-i1 );

				CString	strLabel;
				i1 = strTag.Find( L"aria-label=" );
				if	( i1 >= 0 ){
					i2 = strTag.Find( L"=", i1 );
					i2 += 2;
					i2 = strTag.Find( '"', i2 );
					strLabel = strTag.Mid( i1, i2-i1+1 );
					strLabel += L" ";
				}
				strTag.Format( L"<a target=\"_blank\" %shref=\"%s\">",
							(LPCTSTR)strLabel, (LPCTSTR)strURL );
			}

			// <img>: Remove it except "alt=" text.

			else if	( strElement == L"img" ){
				int	i1 = strTag.Find( L"alt=" );
				if	( i1 >= 0 ){
					i1 += 5;
					int	i2 = strTag.Find( L"\"", i1 );
					if	( i2 >= 0 )
						strTag = strTag.Mid( i1, i2-i1 );
					else
						strTag.Empty();
				}
			}

			// Other tags: Remove them.

			else{
				// Others: Leave element name.

				int	i1 = strTag.Find( L" " );
				int	i2 = strTag.Find( L">" );
				if	( i1 > 0 && i2 > 0 )
					strTag.Delete( i1, i2-i1 );
			}


			if	( !strTag.IsEmpty() )
				strOut += strTag;
		}

		args.xNext = args.xClose + 1;
	}
}

void
CDetag::TrimCopilot( CString& strOut, bool bLast )
{
	// Remove the first "Today".

	if	( strOut.Left( 9 )  == L"\r\nToday\r\n" )
		strOut.Delete( 0, 9 );

	// Trim the last citations.

	else if	( strOut.Right( 6 ) == L"</a>\r\n" ){
		int	iTop = 0;
		int	iA = strOut.GetLength()-6;
		strOut += L"</ul>\r\n";
		for	( ; iA >= 0; iA-- ){

			// Seek the last "<a".

			TCHAR	ch = 0;
			for	( ; iA >= 0; iA-- ){

				if	( strOut[iA] == '<' ){
					ch = strOut[iA+1];
					if	( ch == 'a' )
						break;
					else if	( ch == 'p' )
						;
					else if	( ch == '/' )
						;
					else
						break;
				}
			}

			// Does not hit: break.

			if	( ch != 'a' )
				break;

			CString	strSource,
				strTitle,
				strURL;

			// Take the elements of <a>.

			iTop = iA;
			int	i1 = strOut.Find( L"<p>", iA );
			if	( i1 >= 0 ){
				int	i2 = strOut.Find( L"</p>", i1 );
				if	( i2 >= 0 ){
					strSource = strOut.Mid( i1, i2-i1 );
					strSource.Replace( L"<p>", L"" );
					strSource.Replace( L"\r\n", L"" );
				}
				if	( strOut.Mid( i2, 11 ) == L"</p>\r\n<p>\r\n" ){
					i2 = strOut.Find( L"</p>\r\n", i1 );
					i1 = strOut.Find( L"<p>\r\n", i2 );
					i2 = strOut.Find( L"</p>\r\n", i1 );
					if	( i2 >= 0 ){
						strTitle = strOut.Mid( i1, i2-i1 );
						strTitle.Replace( L"<p>", L"" );
						strTitle.Replace( L"\r\n", L"" );
					}
				}
			}

			i1 = strOut.Find( L"href=", iA );
			if	( i1 >= 0 ){
				i1 += 6;
				int	i2 = strOut.Find( '"', i1 );
				strURL = strOut.Mid( i1, i2-i1 );
				i2 = strURL.Find( L"?utm_source=copilot.com" );
				if	( i2 >= 0 )
					strURL = strURL.Left( i2 );
			}

			i1 = strOut.Find( L"</a>", iA );
			strOut.Delete( iA, i1-iA );
			CString	strLink;
			strLink.Format( L"<a target=\"_blank\" href=\"%s\">%s<br><sup>%s</sup></a>",
				(LPCTSTR)strURL, (LPCTSTR)strTitle, (LPCTSTR)strSource );
			strOut.Insert( iA, strLink );
			strOut.Insert( iA, L"<li> " );
		}
		strOut.Insert( iTop, L"&nbsp;\r\n<ul>\r\n" );
	}

	// Trim queries.

	TrimQandA( strOut );

	strOut += L"<hr>\r\n";
	if	( !bLast )
		strOut += L"<h5>\r\n";
}

void
CDetag::TrimGoogle( CString& strOut, CString strPart )
{
	if	( !strOut.IsEmpty() ){

		// Seek the last </h5>\r\n.

		int	i1 = ReverseFind( strOut, L"</h5>" );
		int	i2 = strOut.Find( '<', i1+1 );
		CString	strNextH5 = strOut.Mid( i2, 4 );

		// Normalize <h6> when it went off the rails.

		if	( strNextH5 != L"<h6>" ){
			int	i0 = strOut.Find( L"<h6>", i2+1 );
			strNextH5 = strOut.Mid( i2, i0-i2 );
			if	( strNextH5.Find( L"<b>\r\nSearching" ) == 0 )
				strOut.Delete( i2, i0-i2 );
			else{
				int	i3 = strOut.Find( L"<h6>", i1+1 );
				if	( i3 >= 0 ){
					CString	strH6 = strOut.Mid( i3 );
					strOut.Delete( i3, strH6.GetLength() );
					strOut.Insert( i2, strH6 );
				}
			}
		}

		// Append the citations at the bottom of the answer.

		ListGoogle( strOut, strPart );

		// Adjust the source chips.

		for	( int i1 = 0;; i1++ ){

			// Seek empty <a></a>.

			i1 = strOut.Find( L">\r\n</a>", i1 );	// \r\n
			if	( i1 < 0 )
				break;

			int	iA = i1-1;
			for	( iA = i1-1; iA > 0; iA-- )
				if	( strOut[iA+0] == '<' &&
					  strOut[iA+1] == 'a' )
					break;
			if	( iA <= 0 )
				break;

			// Remove the empty <a></a>.

			int	cch = i1-iA+7;
			CString	strLink = strOut.Mid( iA, cch );
			strOut.Delete( iA, cch );

			// Make a new <a>...</a>.

			int	x1 = strLink.Find( L"href=\"" );
			int	x2 = strLink.Find( L"\"", x1+6+1 );
			CString	strRef = strLink.Mid( x1, x2-x1+1 );

			x1 = strLink.Find( L"aria-label=" );
			x1 = strLink.Find( L"=",  x1+1 );
			x1 += 2;
			x2 = strLink.Find( L"\"", x1+1 );
			CString	strTitle = strLink.Mid( x1, x2-x1 );
			x1 = strTitle.Find( L"\xff08" );	// Google uses U+FF08 FULLWIDTH LEFT PARENTHESIS
			if	( x1 >= 0 )
				strTitle = strTitle.Left( x1 );
			x1 = strTitle.Find( L" (" );
			if	( x1 >= 0 )
				strTitle = strTitle.Left( x1 );
			x2 = strTitle.Find( L" - " );
			if	( x2 >= 0 )
				strTitle = strTitle.Left( x2 );

			strLink.Format( L"<a target=\"_blank\" %s><sup>%s</sup></a>\r\n", strRef.GetString(), strTitle.GetString() );

			// Insert the new <a>...</a>.

			if	( strOut.Mid( iA-9, 9 ) == L"<p></p>\r\n" )
				iA -= 9;
			strOut.Insert( iA, strLink );
		}

		// Trim queries.

		TrimQandA( strOut );

		// Make footer text below the code list smaller.

		i1 = ReverseFind( strOut, L"</code></pre>\r\n<sup>" );
		if	( i1 < 0 )
			i1 = 0;
		else
			i1 += 21;

		for	( ;; ){
			i1 = SeekTag( strOut, i1,  L"</code></pre>\r\n" );
			if	( i1 < 0 )
				break;
			int	i2 = SeekTag( strOut, i1, L"\r\n" );
			if	( i2 < 0 )
				continue;

			// Make </code></pre>\r\nText to </code></pre>\r\n<sup>Text</sup>\r\n

			if	( strOut[i1-2] == '\r' ){
				strOut.Insert( i1,      L"<sup>" );
				strOut.Insert( i2+5-2, L"</sup>\r\n<p>\r\n" );
			}				
		}

		// Remove empty list items.

		strOut.Replace( L"<li>\r\n</li>\r\n", L"" );
	}

	strOut += L"<hr>\r\n";
}

void
CDetag::ListGoogle( CString& strOut, CString strPart )
{
	// Seek the last list of the citations.

	int	i0 = ReverseFind( (LPCTSTR)strOut, L"\r\n<ul>\r\n<li>\r\n<a" );
	if	( i0 < 0 )
		return;

	// Do not handle a nested list.

	int	i1 = ReverseFind( (LPCTSTR)strOut,  L"<ul>", i0 );
	int	i2 = ReverseFind( (LPCTSTR)strOut, L"</ul>", i0 );
	if	( i1 > i2 )
		return;

	// Remove the (old) list from output.

	i2 = strOut.Find( L"</ul>", i0 );
	CString	strLinks = strOut.Mid( i0, i2-i0 );
	strOut = strOut.Left( i0 );

	CStringArray	saURLs;
	for	( int x = 0;; ){
		x = strLinks.Find( L"href=", x );
		if	( x < 0 )
			break;
		x += 6;
		int	x2 = strLinks.Find( L"\"", x );
		CString	strURL = strLinks.Mid( x, x2-x );
		x = x2;

		INT_PTR	i, n = saURLs.GetCount();
		for	( i = 0; i < n; i++ )
			if	( saURLs[i] == strURL )
				break;
		if	( i >= n )
			saURLs.Add( strURL );
	}

	INT_PTR	n = saURLs.GetCount();
	if	( !n )
		return;

	// Add the (new) list.

	strOut += L"\r\n<ul>&nbsp;\r\n";

	for	( INT_PTR i = 0; i < n; i++ ){

		// Seek <li> where the URL belongs to.

		CString	strURL = saURLs[i];
		CString	strItem;

		int	x1 = -1;
		while	( strItem.IsEmpty() ){
			x1 = ReverseFind( strPart, strURL, x1 );
			if	( x1 < 0 )
				break;

			x1 = ReverseFind( strPart, L"<li", x1 );
			if	( x1 < 0 )
				break;

			int	x2 = strPart.Find( L"</li>", x1 );
			if	( x2 < 0 )
				break;

			CString	strLI = strPart.Mid( x1, x2-x1 );

			int	i1 = strLI.Find( L"href=" );
			if	( i1 < 0 )
				break;
			i1 += 6;
			int	i2 = strLI.Find( '"', i1 );
			if	( i2 < 0 )
				break;
			CString	strRef = strLI.Mid( i1, i2-i1 );
			if	( strRef == strURL )
				strItem = strLI;
		}

		if	( strItem.IsEmpty() )
			continue;

		// Get all <span>s in the <li>.

		CStringArray	saSpans;
		x1 = 0;
		for	( int x = 0; ; ){
			CString	strSpan;
			x1 = strItem.Find( L"<span", x );
			if	( x1 < 0 )
				break;
			int	x2 = SkipBranch( strItem, x1, L"span" );
			if	( x2 < 0 )
				break;
			strSpan = strItem.Mid( x1, x2-x1 );
			x = x2;

			for	( ;; ){
				x1 = strSpan.Find( L"<span" );
				if	( x1 < 0 )
					break;
				x2 = strSpan.Find( L">", x1 );
				strSpan.Delete( x1, x2-x1+1 );
			}
			for	( ;; ){
				x1 = strSpan.Find( L"</span" );
				if	( x1 < 0 )
					break;
				x2 = strSpan.Find( L">", x1 );
				strSpan.Delete( x1, x2-x1+1 );
			}
			for	( ;; ){
				x1 = strSpan.Find( L"<!--" );
				if	( x1 < 0 )
					break;
				x2 = strSpan.Find( L"-->", x1 );
				strSpan.Delete( x1, x2-x1+3 );
			}
			for	( ;; ){
				x1 = strSpan.Find( L"<strong" );
				if	( x1 < 0 )
					break;
				x2 = strSpan.Find( L">", x1 );
				strSpan.Delete( x1, x2-x1+1 );
			}
			for	( ;; ){
				x1 = strSpan.Find( L"</strong" );
				if	( x1 < 0 )
					break;
				x2 = strSpan.Find( L">", x1 );
				strSpan.Delete( x1, x2-x1+1 );
			}
			for	( ;; ){
				x1 = strSpan.Find( L"<svg" );
				if	( x1 < 0 )
					break;
				x2 = strSpan.Find( L"</svg>", x1 );
				strSpan.Delete( x1, x2-x1+6 );
			}
			if	( !strSpan.IsEmpty() )
				saSpans.Add( strSpan );
		}

		if	( saSpans.GetCount() <= 1 ){
			int	x = strLinks.Find( strURL );
			if	( x >= 0 ){
				saSpans.RemoveAll();
				x = strLinks.Find( L"</a>\r\n", x );
				for	( x += 6; x >= 0; ){
					CString	strSpan = strLinks.Tokenize( L"\r\n", x );
					if	( strSpan == L"</li>" )
						break;
					saSpans.Add( strSpan );
				}
			}
		}

		// Introduce the URL with the 1st <span> as title and the last <span> as the source.

		CString	strLink;
		strLink.Format( L"<li><a href=\"%s\" target=\"_blank\">%s<br><sup>%s</sup></a><br>\r\n",
			(LPCTSTR)strURL, (LPCTSTR)saSpans[1], (LPCTSTR)saSpans[0] );
//			(LPCTSTR)strURL, (LPCTSTR)saSpans[saSpans.GetUpperBound()], (LPCTSTR)saSpans[0] );// Old Citations
		strOut += strLink;
	}
	strOut += L"</ul>\r\n";
}

void
CDetag::TrimQandA( CString& strOut )
{
	if	( strOut.IsEmpty() )
		return;

	// Seek the last </h5>\r\n.

	int	x1 = ReverseFind( strOut, L"</h5>" );

	// Decorate the query text there.

	if	( x1 >= 0 ){
		int	x2 = strOut.Find( L"<h6>", x1 );
		if	( x2 >= 0 ){
			// Skip "</h5>\r\n".
			x1 += 7;

			// Replace <h6> to <h5> ( since <h6> is too small ).

			strOut.Delete( x2, 4 );
			strOut.Insert( x2, L"<h5>" );

			CString	strQuery = strOut.Mid( x1, x2-x1 );
			strOut.Delete( x1, x2-x1 );
			TrimQuery( strQuery );
			strOut.Insert( x1, strQuery );

			// Replace </h6> to </h5> ( since <h6> is too small ).

			x2 = strOut.Find( L"</h6>", x2 );
			strOut.Delete( x2, 5 );
			strOut.Insert( x2, L"</h5>" );
		}
	}
}

void
CDetag::TrimQuery( CString& strQuery )
{
	// Remove "Searching" in the query text.
	{
		int	x1 = SeekTag( strQuery, 0, L"<b>" );
		if	( x1 >= 0 ){
			int	x2 = SeekTag( strQuery, x1, L"</b>" );
			if	( x2 >= 0 )
				strQuery.Delete( x1, x2-x1 );
		}
	}
	strQuery.Trim();

	// Remove "File deleted.".

	for	( int x = 0;; x++ ){
		LPCTSTR	pchComment = L"aka.ms";
		int	cchComment = lstrlen( pchComment );

		x = strQuery.Find( pchComment, x );
		if	( x < 0 )
			break;
		int	x1 = x + cchComment;
		int	x2 = strQuery.Find( L"\r\n", x1 );
		strQuery.Delete( x, x2+2-x );
	}

	// Add <br> for each line.

	for	( int x = 0;; ){
		x = strQuery.Find( '\r', x );
		if	( x < 0 )
			break;
		strQuery.Insert( x, L"<br>" );
		x += 5;
	}

	// Enclose in a <table>.

	strQuery.Insert( 0, L"<table style=\"width: 100%; display: table;\"><tr><td>\r\n" );
	strQuery += L"</table>\r\n";
}

int
CDetag::SkipBranch( CString strHTML, int iIndex, CString strElement )
{
	CString	strPlus  = L"<"  + strElement;
	CString	strMinus = L"</" + strElement;
	int	nNest = 1;

	int	x = iIndex + 1;

	for	( int nNest = 1; nNest; x = strHTML.Find( L"<", x ) ){
		int	xPlus  = SeekTag( strHTML, x, (LPCTSTR)strPlus );
		int	xMinus = SeekTag( strHTML, x, (LPCTSTR)strMinus );

		x = -1;
		if	( xPlus < 0 ){
			nNest--;
			x = xMinus;
		}
		if	( xMinus < 0 ){
			nNest++;
			x = xPlus;
		}

		if	( xPlus >= 0 &&
			  xPlus < xMinus ){
			nNest++;
			x = xPlus;
		}
		else if	( xMinus >= 0 &&
			  xMinus < xPlus ){
			nNest--;
			x = xMinus;
		}
		if	( x < 0 )
			break;
	}

	return	x;
}

int
CDetag::IsTagToDelete( CString strHTML, int iIndex, CString strElement )
{
	static	LPCTSTR	apchTags[] = {
		//	element		delete to
			L"button",	L"</button>",
			L"hr",		L">",
			L"label",	L"</label>",
			L"input",	L">",
			L"noscript",	L"</noscript>",
			L"script",	L"</script>",
			L"section",	L">",
			L"/section",	L">",
			L"style",	L"</style>",
			L"svg",		L"</svg>",
			NULL,		NULL
	};

	int	xAfter = -1;
	
	int	iTag = 0;
	for	( iTag = 0; apchTags[iTag]; iTag += 2 ){
		if	( strElement == apchTags[iTag] ){
			xAfter = SeekTag( strHTML, iIndex, apchTags[iTag+1] );
			break;
		}
	}

	return	xAfter;
}

bool
CDetag::IsTagToKeep( CString strElement )
{
	static	LPCTSTR	apchTags[] = {
			L"a",
			L"b", L"strong", L"ins", L"sup", L"sub",
			L"blockquote",
			L"br", L"p",
			L"code", L"pre",
			L"div", L"span",
			L"h1", L"h2", L"h3", L"h5", L"h6",
			L"li", L"ol", L"ul",
			L"table", L"tr", L"td", L"th", L"thead", L"tbody", L"tfoot", L"caption",
			L"textarea",
			L"main",
			L"chat-window", L"user-profile-picture",
			NULL
	};

	if	( strElement[0] == '/' )
		strElement.Delete( 0, 1 );

	for	( int iTag = 0; apchTags[iTag]; iTag++ )
		if	( strElement == apchTags[iTag] )
			return	true;

	return	false;
}

int
CDetag::SeekTag( CString strHTML, int iIndex, LPCTSTR pszTag )
{
	int	xAfter = strHTML.Find( pszTag, iIndex );
	if	( xAfter >= 0 )
		xAfter += lstrlen( pszTag );

	return	xAfter;
}

void
CDetag::OptimizeHTML( CString& strOut )
{
	strOut.Replace( L"<p></p>\r\n\r\n<p>", L"<p></p>\r\n" );
	strOut.Replace( L"</ul>\r\n\r\n<p>",   L"</ul>\r\n" );
	strOut.Replace( L"\r\n\r\n\r\n<p>",    L"\r\n<p>" );
	strOut.Replace( L"<p></p>\r\n</li>",   L"</li>" );
	strOut.Replace( L"<p></p>\r\n ",       L"" );
	strOut.Replace( L"\r\n \r\n",          L"" );
	strOut.Replace( L"\r\n<hr>",           L"\r\n\r\n\r\n<hr>" );
}

int
CDetag::ReverseFind( LPCTSTR pszBase, LPCTSTR pszPattern, int iIndex )
{
	int	nchPattern = lstrlen( pszPattern );
	int	nchBase    = lstrlen( pszBase );
	int	xBase = -1;

	xBase = ( iIndex < 0 )?	nchBase-1: iIndex;
	for	( int x = xBase; x >= 0; x-- ){
		int	nHit = 0;
		for	( nHit = 0; nHit < nchPattern; nHit++ )
			if	( pszBase[x-nHit] != pszPattern[nchPattern-1-nHit] )
				break;
		if	( nHit >= nchPattern )
			return	x-(nHit-1);
	}

	return	-1;
}
