/************************************************************************************
    begin                : Die Sep 16 2003
    copyright            : (C) 2003 by Jeroen Wijnhout (wijnhout@science.uva.nl)
			       2009 by Stefan Hepp (stefan@stefant.org)

    This file is taken from the Kile project and modified to work without Qt on
    the cmdline by Stefan Hepp (stefan@stefant.org).
 ************************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "latexoutputfilter.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <cstdlib>
#include <cctype>
#include <string>
#include <sstream>
#include <iostream>

#include "regex.h"

using namespace std;

////// Helper Functions //////

// Maximum length of a latex log line.
#define MAX_LATEX_LINE_LENGTH  79

static bool file_exists(const string &name)
{
    struct stat info;
    int ret = stat(name.c_str(), &info);

    if ( ret == 0 ) {
	if ( info.st_mode & S_IFDIR ) {
	    return false;
	}
	return true;
    }

    return false;
}

static int parseInt(const string& str)
{
    std::istringstream iss(str);
    int i;
    iss >> i;
    return i;
}

/**
 * Strip whitespace from both ends of a line. Trailing spaces are only removed
 * when trimSpaces is set: the raw log lines keep theirs, because needsSpace()
 * infers latex's line wrapping from the untrimmed line length.
 */
static string trim(const string& str, bool trimSpaces = true) {

    size_t start = str.find_first_not_of(" \t\n\r");

    if ( start == string::npos ) {
        return "";
    }

    size_t end   = str.find_last_not_of(trimSpaces ? " \t\n\r" : "\t\n\r");

    return str.substr( start, end-start+1 );
}

static bool startsWith(const string &str, const string &match) {
    return str.compare(0, match.length(), match) == 0;
}

static bool startsWith(const string &str, char c) {
    return str.length() > 0 && str[0] == c;
}

/**
 * Does this look like a filename TeX would have opened? Used when the file
 * cannot be confirmed on disk, which is the normal case for class and package
 * files that live in the distribution rather than next to the document.
 */
static bool looksLikeFileName(const string &str) {
    size_t dot = str.find_last_of('.');

    if ( dot == string::npos || dot == 0 || dot + 1 == str.length() ) {
	return false;
    }

    // A short alphanumeric extension, so that prose such as "size option"
    // is not mistaken for a file.
    if ( str.length() - dot - 1 > 4 ) {
	return false;
    }

    for (size_t i = dot + 1; i < str.length(); i++) {
	if ( !isalnum((unsigned char)str[i]) ) {
	    return false;
	}
    }

    return true;
}

static bool endsWith(const string &str, char c) {
    return str.length() > 0 && str[str.length()-1] == c;
}


////// Class Code //////

LatexOutputFilter::LatexOutputFilter(const string& source, int verbose, bool nobadboxes, bool quiet) :
    m_nOutputLines(0),
    m_source(source),
    m_verbose(verbose),
    m_nErrors(0),
    m_nWarnings(0),
    m_nBadBoxes(0),
    m_nobadboxes(nobadboxes),
    m_quiet(quiet),
    m_hasPending(false),
    m_pendingVisible(false),
    m_ctxHeadWidth(0),
    m_hasCtxHead(false),
    m_collapse(true),
    m_stream(false),
    m_nShownErrors(0),
    m_nShownWarnings(0),
    m_nShownBadBoxes(0),
    m_debugModel(false),
    m_pretty(false)
{
    // TODO maybe use better method to handle also escaped chars in filename
    size_t pos = source.find_last_of("/\\");
    if ( pos == string::npos ) {
	m_srcPath = ".";
    } else {
	m_srcPath = source.substr(0,pos);
    }
}

LatexOutputFilter::~ LatexOutputFilter()
{
}

bool LatexOutputFilter::fileExists(const string & name)
{
    bool isAbsolute = false;

    // TODO Quick hack to check if filename is an absolute path
    if (name.length() > 1 && (name[0] == '/' || name[1] == ':')) {
	isAbsolute = true;
    }

    if ( isAbsolute ) {
	return file_exists(name);
    }

    string file = "";
    if ( path() != "" && path() != "." ) {
	file = path() + '/';
    }
    file += name;

    if ( file_exists(file) ) {
	return true;
    }

    if ( file_exists(file + ".tex") ) {
	return true;
    }

    return false;
}

bool LatexOutputFilter::needsSpace()
{
    // Heuristic: If the last line was as long as possible, we assume latex did
    // wrap around inside a word.
    return m_nLastLineLength < MAX_LATEX_LINE_LENGTH;
}

// There are basically two ways to detect the current file TeX is processing:
//	1) Use \Input (i.c.w. srctex.sty or srcltx.sty) and \include exclusively. This will
//	cause (La)TeX to print the line ":<+ filename"  in the log file when opening a file,
//	":<-" when closing a file. Filenames pushed on the stack in this mode are marked
//	as reliable.
//
//	2) Since people will probably also use the \input command, we also have to be
//	to detect the old-fashioned way. TeX prints '(filename' when opening a file and a ')'
//	when closing one. It is impossible to detect this with 100% certainty (TeX prints many messages
//	and even text (a context) from the TeX source file, there could be unbalanced parentheses),
//	so we use a heuristic algorithm. In heuristic mode a ')' will only be considered as a signal that
//	TeX is closing a file if the top of the stack is not marked as "reliable".
//	Also, when scanning for a TeX error linenumber (which sometimes causes a context to be printed
//	to the log-file), updateFileStack is not called, helping not to pick up unbalanced parentheses
//	from the context.
void LatexOutputFilter::updateFileStack(const string &strLine, short& dwCookie)
{
	//KILE_DEBUG() << "==LatexOutputFilter::updateFileStack()================" << endl;
	static string strPartialFileName;

	switch (dwCookie) {
		//we're looking for a filename
		case Start : case FileNameHeuristic :
			//TeX is opening a file
			if(startsWith(strLine,":<+ ")) {
// 				KILE_DEBUG() << "filename detected" << endl;
				//grab the filename, it might be a partial name (i.e. continued on the next line)
				strPartialFileName = trim(strLine.substr(4));

				//change the cookie so we remember we aren't sure the filename is complete
				dwCookie = FileName;
			}
			//TeX closed a file
			else if(startsWith(strLine,":<-")) {
// 				KILE_DEBUG() << "\tpopping : " << m_stackFile.top().file() << endl;
				if(!m_stackFile.empty()) {
					m_stackFile.pop();
				}
				dwCookie = Start;
			}
			else {
			//fallback to the heuristic detection of filenames
				updateFileStackHeuristic(strLine, dwCookie);
			}
		break;

		case FileName :
			//The partial filename was followed by '(', this means that TeX is signalling it is
			//opening the file. We are sure the filename is complete now. Don't call updateFileStackHeuristic
			//since we don't want the filename on the stack twice.
			if(startsWith(strLine,'(') || startsWith(strLine,"\\openout")) {
				//push the filename on the stack and mark it as 'reliable'
				m_stackFile.push(LOFStackItem(strPartialFileName, true));
// 				KILE_DEBUG() << "\tpushed : " << strPartialFileName << endl;
				strPartialFileName.clear();
				dwCookie = Start;
			}
			//The partial filename was followed by an TeX error, meaning the file doesn't exist.
			//Don't push it on the stack, instead try to detect the error.
			else if(startsWith(strLine,'!')) {
// 				KILE_DEBUG() << "oops!" << endl;
				dwCookie = Start;
				strPartialFileName.clear();
				detectError(strLine, dwCookie);
			}
			else if(startsWith(strLine,"No file")) {
// 				KILE_DEBUG() << "No file: " << strLine << endl;
				dwCookie = Start;
				strPartialFileName.clear();
				detectWarning(strLine, dwCookie);
			}
			//Partial filename still isn't complete.
			else {
// 				KILE_DEBUG() << "\tpartial file name, adding" << endl;
				strPartialFileName = strPartialFileName + trim(strLine);
			}
		break;

		default:
			break;
	}
}

void LatexOutputFilter::updateFileStackHeuristic(const string &strLine, short & dwCookie)
{
	//KILE_DEBUG() << "==LatexOutputFilter::updateFileStackHeuristic()================";
	static string strPartialFileName;
	bool expectFileName = (dwCookie == FileNameHeuristic);
	int index = 0;

	// handle special case (bug fix for 101810)
	if(expectFileName && strLine.length() > 0 && strLine[0] == ')') {
		m_stackFile.push(LOFStackItem(strPartialFileName));
		expectFileName = false;
		dwCookie = Start;
	}

	//scan for parentheses and grab filenames
	for(unsigned int i = 0; i < strLine.length(); ++i) {
		/*
		We're expecting a filename. If a filename really ends at this position one of the following must be true:
			1) Next character is a space (indicating the end of a filename (yes, there can't spaces in the
			path, this is a TeX limitation).
		comment by tbraun: there is a workround \include{{"file name"}} according to http://groups.google.com/group/comp.text.tex/browse_thread/thread/af873534f0644e4f/cd7e0cdb61a8b837?lnk=st&q=include+space+tex#cd7e0cdb61a8b837,
		but this is currently not supported by kile.
		comment by Stefan Hepp: the directory containing the tex-files *can* contain spaces!
			2) We're at the end of the line, the filename is probably continued on the next line.
			3) The TeX was closed already, signalled by the ')'.
		*/

		bool isLastChar = (i+1 == strLine.length());
		char nextIsTerminator;

		if (isLastChar) {
		    nextIsTerminator = 0;
		} else {
		    char c = strLine[i+1];
		    // TODO check if we are surrounded by " " (i.e. "c:\Documents and Settings\..", if so, wait for next "
		    nextIsTerminator = (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == ')') ? c : (char)0;
		}

		if(expectFileName && (isLastChar || nextIsTerminator)) {
			//KILE_DEBUG() << "Update the partial filename " << strPartialFileName << endl;
			strPartialFileName =  strPartialFileName + strLine.substr(index, i-index + 1);
			// we may continue for dirnames with spaces in it
			index = i + 1;

			if(strPartialFileName.empty()){ // nothing left to do here
			  continue;
			}

			//FIXME: improve these heuristics
			// A '(' that never yields a pushed name is still popped by its
			// matching ')', which drains the stack and eventually discards
			// the real source file. Engines that pack several opens onto
			// one line, such as Tectonic, hit this constantly; pdflatex
			// mostly escapes it by wrapping at 79 columns, so its names
			// land at the end of a line and take the isLastChar path.
			if((isLastChar && (i < 78)) || nextIsTerminator == ')' || nextIsTerminator == '\t'
			   || fileExists(strPartialFileName) || looksLikeFileName(strPartialFileName)) {
				m_stackFile.push(LOFStackItem(strPartialFileName));
				// KILE_DEBUG() << "\tpushed (i = " << i << " length = " << strLine.length() << "): " << strPartialFileName << endl;
				expectFileName = false;
				dwCookie = Start;
			}
			//Guess the filename is continued on the next line, only if the current strPartialFileName does not exist, see bug # 162899
			else if(isLastChar || nextIsTerminator) {
				if(fileExists(strPartialFileName)) {
					m_stackFile.push(LOFStackItem(strPartialFileName));
					//KILE_DEBUG() << "pushed (i = " << i << " length = " << strLine.length() << "): " << strPartialFileName << endl;
					expectFileName = false;
					dwCookie = Start;
				}
				else {
					//KILE_DEBUG() << "Filename spans more than one line." << endl;
					dwCookie = FileNameHeuristic;
				}
			}
			//bail out
			else {
				dwCookie = Start;
				strPartialFileName.clear();
				expectFileName = false;
			}
		}
		//TeX is opening a file
		else if(strLine[i] == '(') {
			//we need to extract the filename
			expectFileName = true;
			strPartialFileName.clear();
			dwCookie = Start;

			//this is were the filename is supposed to start
			index = i + 1;
        	}
		//TeX is closing a file
		else if(strLine[i] == ')') {
			// KILE_DEBUG() << "\tpopping : " << m_stackFile.top().file() << endl;
			//If this filename was pushed on the stack by the reliable ":<+-" method, don't pop
			//a ":<-" will follow. This helps in preventing unbalanced ')' from popping filenames
			//from the stack too soon.
			if(m_stackFile.size() > 1 && !m_stackFile.top().reliable()) {
				m_stackFile.pop();
			}
			else {
				//KILE_DEBUG() << "\t\toh no, forget about it!";
			}
		}
	}
}


void LatexOutputFilter::flushCurrentItem()
{
    //KILE_DEBUG() << "==LatexOutputFilter::flushCurrentItem()================" << endl;
    int nItemType = m_currentItem.type();

    /* TODO why is this needed??
    while( m_stackFile.size() > 0 && !fileExists(m_stackFile.top().file()) ) {
	m_stackFile.pop();
    }
    */

    string sourceFile;
    if ( !m_fileLineSource.empty() ) {
	// file:line:error named the file; trust it over the paren heuristic
	sourceFile = m_fileLineSource;
	m_fileLineSource.clear();
    } else {
	sourceFile = (m_stackFile.empty()) ? source() : m_stackFile.top().file();
    }
    m_currentItem.setSource(sourceFile);

    switch (nItemType) {
	case LatexOutputInfo::itmError:
	    ++m_nErrors;
	    break;

	case LatexOutputInfo::itmWarning:
	    ++m_nWarnings;
	    break;

	case LatexOutputInfo::itmBadBox:
	    ++m_nBadBoxes;
	    break;

	default: break;
    }

    bool visible = nItemType == LatexOutputInfo::itmError
		|| (nItemType == LatexOutputInfo::itmWarning && !m_quiet)
		|| (nItemType == LatexOutputInfo::itmBadBox && !m_nobadboxes);

    // Every item enters the slot, printable or not: the slot has to mean "the
    // item most recently completed", or context following a suppressed warning
    // would attach itself to whatever error came before it.
    emitItem(m_currentItem, visible);

    m_currentItem.clear();
}

/**
 * Longest line TeX will print before clipping. Beyond this the context is a
 * window into the source rather than the whole of it, and the column derived
 * from it does not point anywhere real.
 */
static const size_t MAX_TEX_LINE = 79;

/** How far past an item a context may still arrive and be counted as its own. */
static const int CONTEXT_WINDOW = 12;

/**
 * Recognise the first line of a context block and split off its tag.
 *
 * Three shapes carry one: the source echo "l.<n> ", the reader markers
 * "<recently read> " and friends, and a macro expansion "\foo ->". Everything
 * after the tag is text TeX had already consumed.
 */
static bool splitContextHead(const string& raw, TexContext& context)
{
	static Regex reSourceEcho("^l\\.([0-9]+)( |$)");
	static Regex reMarker("^<[a-zA-Z ]+> ");
	static Regex reExpansion("^\\\\[A-Za-z@]+( #[0-9])* *->");

	if ( reSourceEcho.match(raw) ) {
		context.tag = reSourceEcho.getMatch(raw, 0);
		context.srcLine = parseInt(reSourceEcho.getMatch(raw, 1));
	}
	else if ( reMarker.match(raw) ) {
		context.tag = reMarker.getMatch(raw, 0);
	}
	else if ( reExpansion.match(raw) ) {
		context.tag = reExpansion.getMatch(raw, 0);
	}
	else {
		return false;
	}

	context.before = raw.substr(context.tag.length());

	return true;
}

LatexOutputInfo* LatexOutputFilter::contextTarget()
{
	if ( m_currentItem.isValid() && m_currentItem.type() == LatexOutputInfo::itmError ) {
		return &m_currentItem;
	}

	if ( m_hasPending && m_pendingItem.type() == LatexOutputInfo::itmError
	     && GetCurrentOutputLine() - m_pendingItem.outputLine() <= CONTEXT_WINDOW ) {
		return &m_pendingItem;
	}

	return 0;
}

void LatexOutputFilter::storeContext(TexContext& context)
{
	LatexOutputInfo *target = contextTarget();

	if ( !target ) {
		return;
	}

	// TeX marks text it clipped with an ellipsis, and clips at all only when
	// the line would not fit. Either way what is left is a window, not the
	// line, so the column derived from it cannot be trusted.
	context.windowed =
	    context.tag.length() + context.before.length() + context.after.length() >= MAX_TEX_LINE
	    || context.before.compare(0, 3, "...") == 0
	    || (context.after.length() >= 3
		&& context.after.compare(context.after.length()-3, 3, "...") == 0);

	if ( context.srcLine >= 0 ) {
		target->setSourceContext(context);
	} else {
		target->addTrace(context);
	}
}

void LatexOutputFilter::observeDocument(const string& strLine)
{
	// LaTeX announces each file it has finished reading. The name is the one
	// the author would write in \usepackage, which is what advice has to be
	// checked against.
	static Regex rePackage("^Package: ([^ ]+)");
	static Regex reClass("^Document Class: ([^ ]+)");

	if ( rePackage.match(strLine) ) {
		m_doc.loadedPackages.insert(rePackage.getMatch(strLine, 1));
	}
	else if ( reClass.match(strLine) ) {
		m_doc.loadedClasses.insert(reClass.getMatch(strLine, 1));
	}
}

void LatexOutputFilter::observeContext()
{
	if ( m_hasCtxHead ) {
		size_t indent = m_rawLine.find_first_not_of(' ');
		if ( indent == string::npos ) {
			indent = m_rawLine.length();
		}

		if ( indent == m_ctxHeadWidth ) {
			// Padded to the exact width of the line above: this is the
			// rest of the source line, and the pair is complete.
			m_ctxHead.after = m_rawLine.substr(indent);
			m_hasCtxHead = false;
			storeContext(m_ctxHead);
			return;
		}

		// No continuation, so TeX printed nothing past the error. Keep the
		// half we have, then let this line open a block of its own.
		m_hasCtxHead = false;
		storeContext(m_ctxHead);
	}

	TexContext head;

	if ( !splitContextHead(m_rawLine, head) ) {
		return;
	}

	// Only start tracking when there is something to attach to, so a log full
	// of prose that happens to look like a context cannot accumulate state.
	if ( contextTarget() == 0 ) {
		return;
	}

	m_ctxHead = head;
	m_ctxHeadWidth = m_rawLine.length();
	m_hasCtxHead = true;
}

/** Turn "Missing number, treated as zero" into "Missing number ... x40". */
static string consequenceLine(const LatexOutputInfo& item)
{
    ostringstream text;

    // The headline, not message(): the latter has the error context appended,
    // which is what the classic layout has always printed but is noise here.
    string headline = item.headline();
    size_t end = headline.find_last_not_of(" \t.");
    text << (end == string::npos ? headline : headline.substr(0, end+1));

    if ( item.occurrences() > 1 ) {
	text << " x" << item.occurrences();
    }

    return text.str();
}

void LatexOutputFilter::emitAll()
{
    // Which chains exist is decided on the raw messages, before any grouping.
    // The window that says "this followed from that" was measured in log lines
    // between neighbouring messages; once repeats are collapsed, the surviving
    // pair can be hundreds of lines apart and the window would reject a chain
    // that is plainly there.
    vector<string> consequenceOf(m_items.size());

    if ( m_collapse ) {
	for (size_t i = 0; i < m_items.size(); i++) {
	    for (size_t j = 0; j < i; j++) {
		if ( isConsequenceOf(m_items[j], m_items[i]) ) {
		    // Attach to the start of the chain, not the link before it,
		    // so a consequence of a consequence still names the mistake.
		    consequenceOf[i] = consequenceOf[j].empty()
				     ? groupKey(m_items[j]) : consequenceOf[j];
		    break;
		}
	    }
	}
    }

    // Then collapse repeats. Everything that says the same thing in the same
    // place is one finding, however many times TeX said it.
    vector<string> keys;
    vector<LatexOutputInfo> grouped;
    vector<string> groupedFollows;

    for (size_t i = 0; i < m_items.size(); i++) {
	string key = groupKey(m_items[i]);
	size_t found = keys.size();

	// --no-collapse means every message stands on its own, so nothing is
	// ever matched to an existing group.
	for (size_t j = 0; m_collapse && j < keys.size(); j++) {
	    if ( keys[j] == key ) {
		found = j;
		break;
	    }
	}

	if ( found < keys.size() ) {
	    grouped[found].addOccurrence(m_items[i].sourceLine());
	} else {
	    keys.push_back(key);
	    grouped.push_back(m_items[i]);
	    groupedFollows.push_back(consequenceOf[i]);
	}
    }

    // Hang each chain under the message that started it. Fixing that one
    // removes the rest, so reporting them separately only buries it.
    vector<bool> demoted(grouped.size(), false);

    for (size_t i = 0; i < grouped.size(); i++) {
	if ( groupedFollows[i].empty() ) {
	    continue;
	}

	for (size_t j = 0; j < grouped.size(); j++) {
	    if ( j != i && keys[j] == groupedFollows[i] ) {
		grouped[j].addConsequence(consequenceLine(grouped[i]));
		demoted[i] = true;
		break;
	    }
	}
    }

    for (size_t i = 0; i < grouped.size(); i++) {
	if ( !demoted[i] ) {
	    printItem(grouped[i]);
	}
    }

    m_items.clear();
}

void LatexOutputFilter::getShownCount(int *errors, int *warnings, int *badboxes)
{
    *errors = m_nShownErrors;
    *warnings = m_nShownWarnings;
    *badboxes = m_nShownBadBoxes;
}

void LatexOutputFilter::printItem(LatexOutputInfo& item)
{
    switch (item.type()) {
	case LatexOutputInfo::itmError:   m_nShownErrors++;   break;
	case LatexOutputInfo::itmWarning: m_nShownWarnings++; break;
	case LatexOutputInfo::itmBadBox:  m_nShownBadBoxes++; break;
	default: break;
    }

    if ( m_pretty ) {
	Annotation hint;
	bool hasHint = annotate(item, m_doc, hint);
	cout << renderPretty(item, m_renderOpts, hasHint ? &hint : 0);
    } else {
	cout << item.getMessage();
    }
}

void LatexOutputFilter::emitItem(const LatexOutputInfo& item, bool visible)
{
    releasePending();

    m_pendingItem = item;
    m_pendingVisible = visible;
    m_hasPending = true;
}

/**
 * Print what the parser understood, one field per line. Strings are bracketed
 * because the interesting part of a context is often the whitespace.
 */
static void debugDump(LatexOutputInfo& item)
{
    static const char *severity[] = { "none", "error", "warning", "badbox" };

    int type = item.type();

    cout << "item" << endl;
    cout << "  severity  " << (type >= 0 && type <= 3 ? severity[type] : "?") << endl;
    cout << "  file      " << item.source() << endl;
    cout << "  line      " << item.sourceLine() << endl;
    cout << "  column    " << item.sourceColumn() << endl;
    cout << "  message   [" << item.message() << "]" << endl;

    if ( item.hasSourceContext() ) {
	const TexContext& context = item.sourceContext();
	cout << "  context   tag=[" << context.tag << "] before=[" << context.before
	     << "] after=[" << context.after << "] windowed="
	     << (context.windowed ? 1 : 0) << endl;
	cout << "  recovered [" << context.before << context.after << "]" << endl;
    }

    for (size_t i = 0; i < item.trace().size(); i++) {
	const TexContext& frame = item.trace()[i];
	cout << "  trace[" << i << "]  tag=[" << frame.tag << "] before=["
	     << frame.before << "] after=[" << frame.after << "]" << endl;
    }

    cout << endl;
}

void LatexOutputFilter::releasePending()
{
    if ( !m_hasPending ) {
	return;
    }

    if ( m_debugModel ) {
	debugDump(m_pendingItem);
    }
    else if ( m_pendingVisible ) {
	if ( m_stream ) {
	    printItem(m_pendingItem);
	} else {
	    m_items.push_back(m_pendingItem);
	}
    }

    m_pendingItem.clear();
    m_hasPending = false;
    m_pendingVisible = false;
}

bool LatexOutputFilter::detectError(const string & strLine, short &dwCookie)
{
	//KILE_DEBUG() << "==LatexOutputFilter::detectError(" << strLine << ")================" << endl;

	bool found = false, flush = false;

	static Regex reLaTeXError("^(! )?(LaTeX|pdfTeX|Package|Class) ((.*) )?Error[^:]*: ?(.*)$", true);
	static Regex rePDFLaTeXError("^Error: pdflatex (.*)$", true);
	static Regex reTeXError("^! (.*)\\.$");
	static Regex reLineNumber("^l\\.([0-9]+)(.*)");

	switch (dwCookie) {
		case Start :
			if(reLaTeXError.match(strLine)) {
				//KILE_DEBUG() << "\tError : " <<  reLaTeXError.getMatch(strLine,1) << endl;
				m_currentItem.setMessage(reLaTeXError.getMatch(strLine,5));
				if (!reLaTeXError.getMatch(strLine,4).empty()) {
				    m_currentItem.setPackage(reLaTeXError.getMatch(strLine,2), reLaTeXError.getMatch(strLine,4));
				}
				found = true;
			}
			else if(rePDFLaTeXError.match(strLine)) {
				//KILE_DEBUG() << "\tError : " <<  rePDFLaTeXError.getMatch(strLine,1) << endl;
				m_currentItem.setMessage(rePDFLaTeXError.getMatch(strLine,1));
				found = true;
			}
			else if(reTeXError.match(strLine)) {
				//KILE_DEBUG() << "\tError : " <<  reTeXError.getMatch(strLine,1) << endl;
				m_currentItem.setMessage(reTeXError.getMatch(strLine,1));
				found = true;
			}
			if(found) {
				dwCookie = endsWith(strLine,'.') ? LineNumber : Error;
			}
		break;

		case Error :
			//KILE_DEBUG() << "\tError (cont'd): " << strLine << endl;
			if(endsWith(strLine,'.')) {
				dwCookie = LineNumber;
				m_currentItem.addMessage(strLine, needsSpace());
			}
			else if(GetCurrentOutputLine() - m_currentItem.outputLine() > 5) {
				cerr << "\tBAILING OUT: error description spans more than five lines" << endl;
				dwCookie = Start;
				flush = true;
			}
		break;

		case LineNumber :
			//KILE_DEBUG() << "\tLineNumber " << endl;
			if(reLineNumber.match(strLine)) {
				dwCookie = Start;
				flush = true;
				//KILE_DEBUG() << "\tline number: " << reLineNumber.getMatch(strLine,1) << endl;
				m_currentItem.setSourceLine(parseInt(reLineNumber.getMatch(strLine,1)));
				// TODO Does latex wrap around into a new line that starts with 'l.'? Assuming it
				//      does not (we always add a space).
				// Context, not complaint: observeContext() has already
				// taken this apart properly, so keep it out of the headline.
				m_currentItem.addMessage(reLineNumber.getMatch(strLine,2), true, false);
			}
			else if(GetCurrentOutputLine() - m_currentItem.outputLine() > 10) {
				dwCookie = Start;
				flush = true;
				cerr << "\tBAILING OUT: did not detect a TeX line number for an error" << endl;
				m_currentItem.setSourceLine(0);
			} else {
				// Everything between the complaint and the "l.<n>" echo is
				// the expansion trace, which the headline does not want.
				m_currentItem.addMessage(strLine, needsSpace(), false);
			}
		break;

		default : break;
	}

	if(found) {
		m_currentItem.setType(LatexOutputInfo::itmError);
		m_currentItem.setOutputLine(GetCurrentOutputLine());
	}

	if(flush) {
		flushCurrentItem();
	}

	return found;
}

/**
 * Errors in "file:line:error" style, produced by -file-line-error and emitted
 * unconditionally by Tectonic. The format states the file and the line outright,
 * so this bypasses both the parenthesis heuristic that tracks the current file
 * and the scan for a following "l.<n>" context line.
 */
bool LatexOutputFilter::detectFileLineError(const string & strLine, short & dwCookie)
{
	// The file part is greedy so that a drive letter or any other colon in
	// the path stays with the filename; only the last ":<digits>: " counts.
	static Regex reFileLineError("^(.+):([0-9]+): (.+)$");

	if(!reFileLineError.match(strLine)) {
		return false;
	}

	m_fileLineSource = reFileLineError.getMatch(strLine, 1);

	m_currentItem.setType(LatexOutputInfo::itmError);
	m_currentItem.setOutputLine(GetCurrentOutputLine());
	m_currentItem.setSourceLine(parseInt(reFileLineError.getMatch(strLine, 2)));
	m_currentItem.setMessage(reFileLineError.getMatch(strLine, 3));

	// Everything needed is on this one line, so emit it rather than waiting
	// for a line number that this format never prints.
	dwCookie = Start;
	flushCurrentItem();

	return true;
}

bool LatexOutputFilter::detectWarning(const string & strLine, short &dwCookie)
{
	//KILE_DEBUG() << "==LatexOutputFilter::detectWarning(" << strLine.length() << ")================" << endl;

// TODO: correctly match: (add package name to msg, remove '(<package>)      ' from following line
// Package hyperref Warning: Option `pdfpagelabels' is turned off
// (hyperref)                because \thepage is undefined.
//
// TODO match multiline, multiparagraph messages (?)

	bool found = false, flush = false;
	string warning;

	static Regex reLaTeXWarning("^(! )?(LaTeX|pdfTeX|Package|Class) ((.*) )?Warning[^:]*: ?(.*)$", true);
	static Regex reNoFile("No file (.*)");
	// FIXME can be removed when http://sourceforge.net/tracker/index.php?func=detail&aid=1772022&group_id=120000&atid=685683 has promoted to the users
	static Regex reNoAsyFile("File .* does not exist.");

	switch(dwCookie) {
	    //detect the beginning of a warning
	    case Start :
		if(reLaTeXWarning.match(strLine)) {
		    warning = reLaTeXWarning.getMatch(strLine,5);
		    //KILE_DEBUG() << "\tWarning found: " << warning << endl;

		    found = true;
		    dwCookie = Start;

		    m_currentItem.setMessage(warning);
		    m_currentItem.setOutputLine(GetCurrentOutputLine());

		    if (!reLaTeXWarning.getMatch(strLine,4).empty()) {
			m_currentItem.setPackage(reLaTeXWarning.getMatch(strLine,2), reLaTeXWarning.getMatch(strLine,4));
		    }

		    //do we expect a line number?
		    flush = detectLaTeXLineNumber(warning, dwCookie, strLine.length());
		}
		else if(reNoFile.match(strLine)) {
		    found = true;
		    flush = true;
		    m_currentItem.setSourceLine(0);
		    m_currentItem.setMessage(reNoFile.getMatch(strLine,0));
		    m_currentItem.setOutputLine(GetCurrentOutputLine());
		}
		else if(reNoAsyFile.match(strLine)) {
		    found = true;
		    flush = true;
		    m_currentItem.setSourceLine(0);
		    m_currentItem.setMessage(reNoAsyFile.getMatch(strLine,0));
		    m_currentItem.setOutputLine(GetCurrentOutputLine());
		}

		break;

	    //warning spans multiple lines, detect the end
	    case Warning :
	    {
		// The line-number scan needs the text of this continuation
		// line; it used to receive the empty `warning` local, so a
		// warning that carries its "on input line N." on a later line
		// never got a source line. Strip trailing spaces first, the
		// end-of-line anchors cannot see past them.
		string cont = strLine;
		size_t end = cont.find_last_not_of(" \t");
		cont = (end == string::npos) ? "" : cont.substr(0, end+1);

		flush = detectLaTeXLineNumber(cont, dwCookie, strLine.length());
		m_currentItem.addMessage(strLine, needsSpace());
	    }
		break;

	    default:
		break;
	}

	if(found) {
	    m_currentItem.setType(LatexOutputInfo::itmWarning);
	    m_currentItem.setOutputLine(GetCurrentOutputLine());
	}

	if(flush) {
	    flushCurrentItem();
	}

	return found;
}

bool LatexOutputFilter::detectLaTeXLineNumber(string & warning, short & dwCookie, size_t len)
{
	//KILE_DEBUG() << "==LatexOutputFilter::detectLaTeXLineNumber(" << warning.length() << ")================" << endl;

	static Regex reLaTeXLineNumber("(.*) on input line ([0-9]+)\\.$", true);
	// The greedy (.*) must not eat into the number: on a line wrapped
	// mid-word such as "e 13." it left only the final digit in group 2,
	// reporting line 3 for line 13.
	static Regex reInternationalLaTeXLineNumber("(.*[^0-9])([0-9]+)\\.$", true);

	if(reLaTeXLineNumber.match(warning)) {
		//KILE_DEBUG() << "een" << endl;
		m_currentItem.setSourceLine(parseInt(reLaTeXLineNumber.getMatch(warning,2)));
		warning = reLaTeXLineNumber.getMatch(warning,1);
		dwCookie = Start;
		return true;
	}
	else if(reInternationalLaTeXLineNumber.match(warning)) {
		//KILE_DEBUG() << "een" << endl;
		m_currentItem.setSourceLine(parseInt(reInternationalLaTeXLineNumber.getMatch(warning,2)));
		dwCookie = Start;
		return true;
	}
	else if(endsWith(warning,'.')) {
		//KILE_DEBUG() << "twee" << endl;
		m_currentItem.setSourceLine(-1);
		dwCookie = Start;
		return true;
	}
	//bailing out, did not find a line number
	else if((GetCurrentOutputLine() - m_currentItem.outputLine() > 8) || (len == 0)) {
		//KILE_DEBUG() << "drie current " << GetCurrentOutputLine() << " " <<  m_currentItem.outputLine() << " len " << len << endl;
		m_currentItem.setSourceLine(-1);
		dwCookie = Start;
		return true;
	}
	//error message is continued on the other line
	else {
		//KILE_DEBUG() << "vier" << endl;
		dwCookie = Warning;
		return false;
	}
}

bool LatexOutputFilter::detectBadBox(const string & strLine, short & dwCookie)
{
	//KILE_DEBUG() << "==LatexOutputFilter::detectBadBox(" << strLine.length() << ")================" << endl;

	bool found = false, flush = false;
	string badbox;

	static Regex reBadBox("^(Over|Under)(full \\\\[hv]box .*)", true);

	switch(dwCookie) {
	    case Start :
		if(reBadBox.match(strLine)) {
			found = true;
			dwCookie = Start;
			badbox = strLine;
			flush = detectBadBoxLineNumber(badbox, dwCookie, strLine.length());
			m_currentItem.setMessage(badbox);
		}
		break;

	    case BadBox :
		badbox = m_currentItem.message() + strLine;
		flush = detectBadBoxLineNumber(badbox, dwCookie, strLine.length());
		m_currentItem.setMessage(badbox);
		break;

	    default:
		    break;
	}

	if(found) {
	    m_currentItem.setType(LatexOutputInfo::itmBadBox);
	    m_currentItem.setOutputLine(GetCurrentOutputLine());
	}

	if(flush) {
	    flushCurrentItem();
	}

	return found;
}

bool LatexOutputFilter::detectBadBoxLineNumber(string & strLine, short & dwCookie, size_t len)
{
	//KILE_DEBUG() << "==LatexOutputFilter::detectBadBoxLineNumber(" << strLine.length() << ")================" << endl;

	static Regex reBadBoxLines("(.*) at lines ([0-9]+)--([0-9]+)", true);
	static Regex reBadBoxLine("(.*) at line ([0-9]+)", true);
	// A badbox reported as "has occurred while \output is active" carries no
	// source line, because TeX does not report one. A pattern for it used to
	// live here, anchored as "...is active^", which could never match. The
	// bail-out below already assigns line 0 to these, so nothing was lost.
	// Reinstating the pattern is not worth it: its branch also read capture
	// groups from reBadBoxLines, which returns stale offsets from an earlier
	// match when reBadBoxLines itself did not match.

	string match = strLine;

	if(reBadBoxLines.match(strLine)) {
		dwCookie = Start;
		strLine = reBadBoxLines.getMatch(match,1);
		int n1 = parseInt(reBadBoxLines.getMatch(match,2));
		int n2 = parseInt(reBadBoxLines.getMatch(match,3));
		m_currentItem.setSourceLine(n1 < n2 ? n1 : n2);
		return true;
	}
	else if(reBadBoxLine.match(strLine)) {
		dwCookie = Start;
		strLine = reBadBoxLine.getMatch(match,1);
		m_currentItem.setSourceLine(parseInt(reBadBoxLine.getMatch(match,2)));
		//KILE_DEBUG() << "\tBadBox@" << reBadBoxLine.getMatch(strLine,2) << "." << endl;
		return true;
	}
	//bailing out, did not find a line number
	else if((GetCurrentOutputLine() - m_currentItem.outputLine() > 3) || (len == 0)) {
		dwCookie = Start;
		m_currentItem.setSourceLine(0);
		return true;
	}
	else {
		dwCookie = BadBox;
	}

	return false;
}

short LatexOutputFilter::parseLine(const string & strLine, short dwCookie)
{
	//KILE_DEBUG() << "==LatexOutputFilter::parseLine(" << strLine << dwCookie << strLine.length() << ")================" << endl;

	// Ahead of the detectors, because an "l.<n>" line is both the second half
	// of a context block and the signal that ends the error it belongs to.
	observeContext();
	observeDocument(strLine);

	switch (dwCookie) {
		case Start :
			if(!(detectBadBox(strLine, dwCookie) || detectWarning(strLine, dwCookie)
			  || detectError(strLine, dwCookie) || detectFileLineError(strLine, dwCookie))) {
				updateFileStack(strLine, dwCookie);
			}
		break;

		case Warning :
			detectWarning(strLine, dwCookie);
		break;

		case Error : case LineNumber :
			detectError(strLine, dwCookie);
		break;

		case BadBox :
			detectBadBox(strLine, dwCookie);
		break;

		case FileName : case FileNameHeuristic :
			updateFileStack(strLine, dwCookie);
		break;

		default:
			dwCookie = Start;
			break;
	}

	m_nLastLineLength = strLine.length();

	return dwCookie;
}

bool LatexOutputFilter::run(FILE *out)
{
	m_nErrors = m_nWarnings = m_nBadBoxes = 0;
	m_fileLineSource.clear();
	m_nLastLineLength = 0;
	m_hasPending = false;
	m_pendingVisible = false;
	m_pendingItem.clear();
	m_rawLine.clear();
	m_hasCtxHead = false;
	m_ctxHeadWidth = 0;
	m_doc.loadedPackages.clear();
	m_doc.loadedClasses.clear();
	m_items.clear();
	m_nShownErrors = m_nShownWarnings = m_nShownBadBoxes = 0;
	while (!m_stackFile.empty()) {
	    m_stackFile.pop();
	}
	m_stackFile.push(LOFStackItem(source()));

	m_nOutputLines = 0;

	short sCookie = 0;
	string s = "";

	char line[120];
	line[119] = line[118] = 0;

	while ( fgets(line, sizeof(line), out) ) {

	    s += line;

	    if ( line[118] != 0 && line[118] != '\n' ) {
		// line is too long, continue reading
		line[118] = 0;
		continue;
	    }

	    if ( m_verbose ) {
		cerr << s;
	    }

	    // Keep the line as it arrived, minus the line ending. parseLine gets
	    // a trimmed copy; the indent it throws away is what encodes the
	    // column of a TeX error context.
	    m_rawLine = s;
	    size_t end = m_rawLine.find_last_not_of("\n\r");
	    m_rawLine = (end == string::npos) ? "" : m_rawLine.substr(0, end+1);

	    sCookie = parseLine(trim(s, false), sCookie);
	    ++m_nOutputLines;

	    s.clear();
	}

	bool ret = true;

	if ( ferror(out) ) {
	    perror("Parsing stdout");
	    ret = false;
	}

	if ( m_currentItem.isValid() ) {
	    flushCurrentItem();
	}

	releasePending();
	emitAll();

	if ( m_debugModel ) {
	    cout << "document" << endl;
	    cout << "  classes   " << m_doc.loadedClasses.size() << endl;
	    cout << "  packages  " << m_doc.loadedPackages.size() << endl;
	}

	return ret;
}

/** Return number of errors etc. found in log-file. */
void LatexOutputFilter::getErrorCount(int *errors, int *warnings, int *badboxes)
{
    *errors = m_nErrors;
    *warnings = m_nWarnings;
    *badboxes = m_nBadBoxes;
}
