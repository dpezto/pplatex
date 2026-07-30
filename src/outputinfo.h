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
};

#endif
