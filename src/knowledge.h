/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef __PPLATEX_KNOWLEDGE_H__
#define __PPLATEX_KNOWLEDGE_H__

#include <set>
#include <string>

#include "outputinfo.h"

/**
 * What the log says about the document as a whole rather than about any one
 * message: which packages and classes actually loaded.
 *
 * This is what keeps advice honest. The obvious hint for an undefined
 * \superseteq is "add \usepackage{amssymb}", and in the log that error comes
 * from, amssymb is already loaded -- so the obvious hint is wrong, and wrong
 * in the way that costs the reader a compile to find out.
 */
struct DocContext
{
	std::set<std::string> loadedPackages;
	std::set<std::string> loadedClasses;

	/** True if the document already loaded <name>, as a package or a class. */
	bool loaded(const std::string& name) const {
		return loadedPackages.find(name) != loadedPackages.end()
		    || loadedClasses.find(name) != loadedClasses.end();
	}
};

/**
 * How far a hint can be trusted, which the reader gets to see.
 *
 * Nothing here is presented as more certain than it is. A hint that restates
 * a rule of the format cannot be wrong; a hint that names a package to install
 * rests on a table and on what the log happened to record, and says so.
 */
enum Confidence
{
	/** A mechanical fact about the format. Cannot be wrong. */
	CONF_CERTAIN,
	/** The usual cause, but not the only one. */
	CONF_LIKELY,
	/** Derived, and may not describe this document. */
	CONF_GUESS
};

/** A piece of advice attached to a message. At most two lines, 76 columns. */
struct Annotation
{
	Annotation() : confidence(CONF_LIKELY) {}

	bool empty() const { return first.empty(); }

	/** "hint", "likely" or "maybe", matching the confidence. */
	const char *label() const;

	Confidence confidence;
	std::string first;
	std::string second;
};

/**
 * Work out what to tell the reader about <item>, given what <doc> says the
 * document loaded. Returns false when there is nothing worth saying, which is
 * the right answer more often than not: a wrong hint costs more than no hint,
 * because it sends the reader off to change something that was never broken.
 */
bool annotate(const LatexOutputInfo& item, const DocContext& doc, Annotation& out);

/**
 * True if <follower> only happened because <root> did.
 *
 * An undefined command expands to nothing, so the dimension that was supposed
 * to follow it is missing, so the unit after that is missing too: one mistake,
 * three errors. Reporting all three as separate problems buries the only one
 * worth fixing. Each rule here names a mechanism, not a correlation.
 */
bool isConsequenceOf(const LatexOutputInfo& root, const LatexOutputInfo& follower);

/**
 * A key identifying messages that say the same thing, for reporting repeats
 * once. Badbox amounts are left out of it, so that the same overfull line at
 * two sizes counts as one finding; everything else compares exactly, because
 * numbers in an error are usually the part that matters.
 */
std::string groupKey(const LatexOutputInfo& item);

/**
 * Check the built-in tables for internal consistency and print what is wrong.
 * Returns the number of problems found. Exposed so that --self-test can gate
 * a release on it.
 */
int selfTestKnowledge();

#endif
