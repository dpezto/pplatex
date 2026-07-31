/************************************************************************************
    begin                : Die Sep 16 2003
    copyright            : (C) 2003 by Jeroen Wijnhout (wijnhout@science.uva.nl)
                               2008 by Michel Ludwig (michel.ludwig@kdemail.net)
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

#include "outputinfo.h"

#include <string>
#include <sstream>

#include "regex.h"

using namespace std;

LatexOutputInfo::LatexOutputInfo()
{
    clear();
}

void LatexOutputInfo::clear()
{
    m_strSrcFile.clear();
    m_nSrcLine = -1;
    m_nOutputLine = -1;
    m_strError.clear();
    m_nErrorID = -1;
    m_msgClass.clear();
    m_package.clear();
    m_srcContext = TexContext();
    m_hasSrcContext = false;
    m_nSrcColumn = -1;
    m_trace.clear();
}

void LatexOutputInfo::setSourceContext(const TexContext& context)
{
    m_srcContext = context;
    m_hasSrcContext = true;

    // The column is the width of the text TeX had consumed, counting from 1.
    // Where TeX clipped the line to fit, that width measures the clipped text
    // and means nothing in the real file, so report no column at all.
    m_nSrcColumn = context.windowed ? -1 : (int)context.before.length() + 1;
}

bool LatexOutputInfo::isValid() const
{
    return !(m_strSrcFile.empty() && m_nSrcLine == -1 && m_nOutputLine == -1
				  && m_strError.empty() && m_nErrorID == -1);
}

void LatexOutputInfo::addMessage(const string& msg, bool addSpace)
{
    // Lines reach us with their trailing spaces intact, because the length of
    // an untrimmed line is what tells the filter whether latex wrapped a word.
    // Compare without them, or filler padded out to the width of the log slips
    // through: "Type  H <return>..." carries none and was dropped, while the
    // "..." continuation marker carries dozens and was not.
    string text = msg;
    size_t end = text.find_last_not_of(" \t");
    text = (end == string::npos) ? "" : text.substr(0, end+1);

    if (text.empty() ||
	text == "Type  H <return>  for immediate help." ||
	text == "...")
    {
	return;
    }

    const string regex = "^(\\(" + m_package + "\\))? *(.*)$";

    Regex regLine(regex.c_str());
    
    string line;

    if ( regLine.match(msg) ) {
	line = regLine.getMatch(msg, 2);
    } else {
	line = msg;
    }

    if (m_strError.length() + line.length() + (addSpace ? 1 : 0) < 80) {
	m_strError = m_strError + (addSpace ? " " : "") + line;
    } else {
	m_strError = m_strError + "\n   " + line;
    }
}

string LatexOutputInfo::getMessage()
{
    ostringstream msg;
    bool hasType = true;
    
    switch (m_nErrorID) {
	case itmError:   msg << "** Error  ";   break;
	case itmWarning: msg << "** Warning"; break;
	case itmBadBox:  msg << "** BadBox ";  break;
	default: hasType = false;
    }

    if ( !m_strSrcFile.empty() ) {
	if ( hasType ) {
	    msg << " in ";
	}
	msg << m_strSrcFile;
    }

    if ( m_nSrcLine > 0 ) {
	msg << ", Line " << m_nSrcLine;
    }

    msg << ": ";

    if (msg.str().length() + m_msgClass.length() + m_package.length() + m_strError.length() > 78 && !m_strError.empty() ) {
	msg << endl << "   ";
    }

    if (!m_msgClass.empty() ) {
	msg << "(" << m_msgClass << " " << m_package << ") ";
    }

    msg << m_strError << endl;

    msg << endl;

    return msg.str();
}

