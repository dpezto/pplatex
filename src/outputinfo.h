/**********************************************************************************
    begin                : Die Sep 16 2003
    copyright            : (C) 2003 by Jeroen Wijnhout (wijnhout@science.uva.nl)
                               2008 by Michel Ludwig (michel.ludwig@kdemail.net)
			       2009 by Stefan Hepp (stefan@stefant.org)

    This file is taken from the Kile project and modified to work without Qt on
    the cmdline by Stefan Hepp (stefan@stefant.org).
 **********************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OUTPUTINFO_H
#define OUTPUTINFO_H

#include <string>
#include <vector>

/**
 * One of the two-line blocks TeX prints to show where it stopped reading.
 *
 *     l.3 \zzz
 *             {aa}
 *
 * The first line carries a tag and the text TeX had already consumed; the
 * second is padded out to exactly the width of the first. Splitting the pair
 * apart therefore recovers both the source line -- "\zzz" + "{aa}" is what
 * min.tex line 3 actually says -- and the column TeX stopped at, which is the
 * width of everything before the padding ends.
 *
 * The same shape carries the expansion trace, tagged "<recently read> " or
 * "\Gm@lmargin ->" instead of "l.<n> ".
 **/
struct TexContext
{
	TexContext() : srcLine(-1), windowed(false) {}

	/** "l.3 ", "<recently read> ", "\Gm@lmargin ->" */
	std::string tag;
	/** Text to the left of the point TeX stopped at. */
	std::string before;
	/** Text to the right of it, empty when TeX printed no continuation. */
	std::string after;
	/** Line number carried by an "l.<n>" tag, -1 for a trace frame. */
	int srcLine;
	/** TeX clipped the line to fit its own width, so the column is a guess. */
	bool windowed;
};

/**
 * A single message parsed out of the LaTeX output, and the formatting of that
 * message for the console.
 *
 * @author Thorsten Lck
 * @author Jeroen Wijnhout
 * @author Stefan Hepp
 **/

class LatexOutputInfo
{
    public:
	/**
	 * These constants are describing, which item type is currently
	 * parsed. (to be set as error code)
	 **/
	enum tagCookies
	{
		itmNone = 0,
		itmError,
		itmWarning,
		itmBadBox
	};

	/**
	 * Constructs an invalid output information object.
	 **/
	LatexOutputInfo();

	/**
	 * Returns true if and only if this object contains valid output
	 * information.
	 **/
	bool isValid() const;

	/** Source file where error occurred. */
	std::string source() const { return m_strSrcFile; }
	/** Source file where error occurred. */
	void setSource(const std::string& src) { m_strSrcFile = src; }

	/** Line number in source file of the current message */
	void setSourceLine(int line) { m_nSrcLine =  line; }
	/** Line number in source file of the current message, -1 if unknown. */
	int sourceLine() const { return m_nSrcLine; }

	/**
	 * Column TeX stopped at, counting from 1, or -1 when it cannot be
	 * trusted. Derived from the width of the context, so it is only as good
	 * as the context: a line TeX clipped to fit its own output width gives
	 * a column into the clipped text, which would send an editor to the
	 * wrong place. Those report -1 instead.
	 **/
	int sourceColumn() const { return m_nSrcColumn; }

	bool hasSourceContext() const { return m_hasSrcContext; }
	const TexContext& sourceContext() const { return m_srcContext; }
	void setSourceContext(const TexContext& context);

	/** Expansion frames, outermost first, as TeX printed them. */
	const std::vector<TexContext>& trace() const { return m_trace; }
	void addTrace(const TexContext& context) { m_trace.push_back(context); }

	/** Error message */
	const std::string message() const { return m_strError; }

	/** Error message */
	void setMessage(const std::string& message) { m_strError = message; }

	void setPackage(const std::string& msgClass, const std::string& package) { m_msgClass = msgClass; m_package = package; }

	/** Error code */
	int type() const { return m_nErrorID; }
	/** Error code */
	void setType(int type) { m_nErrorID = type; }

	/** Line number in the output, where error was reported. */
	int outputLine() const { return m_nOutputLine; }
	/** Line number in the output, where error was reported. */
	void setOutputLine(int line) { m_nOutputLine = line; }

	/**
	 * Clears the information stored in this object, turning it
	 * into an invalid output information object.
	 **/
	void clear();

	/** The message as it is printed to the console, including a trailing blank line. */
	std::string getMessage();

	void addMessage(const std::string& msg, bool addSpace = true);

    private:
	std::string m_strSrcFile;
	int m_nSrcLine;
	std::string m_strError;
	std::string m_msgClass;
	std::string m_package;
	int m_nOutputLine;
	int m_nErrorID;

	TexContext m_srcContext;
	bool m_hasSrcContext;
	int m_nSrcColumn;
	std::vector<TexContext> m_trace;
};

#endif
