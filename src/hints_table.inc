/* What to do about a message, keyed on what LaTeX said.
 *
 * Hand written, and meant to be added to: one entry is a few lines, greppable
 * by the text it matches, and every entry carries an example that --self-test
 * runs against the table. The check that matters is that an example matches
 * its own entry and no earlier one, which catches a general pattern shadowing
 * a specific one -- the way this table breaks as it grows.
 *
 * Wording rules, enforced by review and partly by --self-test:
 *
 *   Imperative, and name the command, package or flag literally.
 *   Two lines at most, 76 columns at most.
 *   No "consider", no "you might want to", no "check your packages".
 *   State the mechanism in a clause, then the action.
 *   A hint that would be wrong more than about one time in five stays out.
 *
 * Matching is against the message text with any trailing full stop removed.
 * 'P' is a prefix, 'S' a substring, 'R' a POSIX regex. First match wins, so
 * specific entries come before general ones.
 *
 * SEV_ANY matches any severity; a package name restricts an entry to messages
 * that package raised.
 */

#define SEV_ANY (-1)

static const struct {
    const char *id;
    int severity;
    const char *package;
    char kind;
    const char *pattern;
    Confidence confidence;
    const char *first;
    const char *second;
    const char *example;
} HINTS[] = {

/* --- TeX primitives: the head of the distribution ------------------------ */

{ "missing-dollar", LatexOutputInfo::itmError, "", 'P', "Missing $ inserted", CONF_LIKELY,
  "A math-only command was used in ordinary text. Wrap it in $...$,",
  "or an earlier $ was never closed.",
  "Missing $ inserted" },

{ "missing-number", LatexOutputInfo::itmError, "", 'P', "Missing number, treated as zero", CONF_LIKELY,
  "A length or counter argument came out empty. Usually a consequence",
  "of an undefined command just above.",
  "Missing number, treated as zero" },

{ "illegal-unit", LatexOutputInfo::itmError, "", 'P', "Illegal unit of measure", CONF_LIKELY,
  "A dimension has no unit: write 3cm, not 3. Often a consequence of",
  "the error just above.",
  "Illegal unit of measure (pt inserted)" },

{ "missing-begin-document", LatexOutputInfo::itmError, "", 'P', "Missing \\begin{document}", CONF_LIKELY,
  "Something typeset text before \\begin{document}. Look for a stray",
  "character or a non-ASCII byte in the preamble.",
  "Missing \\begin{document}" },

{ "missing-endcsname", LatexOutputInfo::itmError, "", 'P', "Missing \\endcsname inserted", CONF_LIKELY,
  "\\csname was handed something that is not a plain name, usually an",
  "undefined command inside \\ref, \\cite or \\newcommand.",
  "Missing \\endcsname inserted" },

{ "missing-open-brace", LatexOutputInfo::itmError, "", 'P', "Missing { inserted", CONF_CERTAIN,
  "TeX inserted a { to carry on. An earlier } closed a group too soon;",
  "check the braces on this line.",
  "Missing { inserted" },

{ "missing-close-brace", LatexOutputInfo::itmError, "", 'P', "Missing } inserted", CONF_CERTAIN,
  "A group was left open. Check the braces of the argument ending here,",
  "and \\left without a matching \\right.",
  "Missing } inserted" },

{ "too-many-braces", LatexOutputInfo::itmError, "", 'P', "Too many }'s", CONF_CERTAIN,
  "One } more than {. Delete it, or add the { it was meant to close.",
  "",
  "Too many }'s" },

{ "extra-brace-or-dollar", LatexOutputInfo::itmError, "", 'P', "Extra }, or forgotten $", CONF_LIKELY,
  "A } closed a group that math mode had opened. A $ is missing before it.",
  "",
  "Extra }, or forgotten $" },

{ "extra-alignment-tab", LatexOutputInfo::itmError, "", 'P', "Extra alignment tab", CONF_CERTAIN,
  "This row has more & than the column spec allows. Add a column to",
  "\\begin{tabular}{...}, or delete the extra &.",
  "Extra alignment tab has been changed to \\cr" },

{ "misplaced-tab", LatexOutputInfo::itmError, "", 'P', "Misplaced alignment tab character &", CONF_CERTAIN,
  "& outside a table or array. Write \\& for a literal ampersand.",
  "",
  "Misplaced alignment tab character &" },

{ "misplaced-noalign", LatexOutputInfo::itmError, "", 'P', "Misplaced \\noalign", CONF_CERTAIN,
  "\\hline and \\noalign may only follow \\\\ or start a row.",
  "",
  "Misplaced \\noalign" },

{ "paragraph-ended", LatexOutputInfo::itmError, "", 'R', "^Paragraph ended before (\\\\[A-Za-z@]+) was complete", CONF_CERTAIN,
  "A blank line ended \\1's argument. Remove the blank line, or declare",
  "\\1 with \\newcommand, which allows one, instead of \\def.",
  "Paragraph ended before \\foo was complete" },

{ "runaway-argument", LatexOutputInfo::itmError, "", 'P', "Runaway argument", CONF_CERTAIN,
  "A macro argument was never closed; a } is missing. The text TeX shows",
  "is what it swallowed, so the mistake is just before it.",
  "Runaway argument" },

{ "file-ended-scanning-use", LatexOutputInfo::itmError, "", 'R', "^File ended while scanning use of (\\\\[A-Za-z@]+)", CONF_CERTAIN,
  "\\1's argument is never closed. A } is missing between \\1 and the",
  "end of the file.",
  "File ended while scanning use of \\foo" },

{ "file-ended-scanning-text", LatexOutputInfo::itmError, "", 'P', "File ended while scanning text of", CONF_CERTAIN,
  "An environment or group runs to the end of the file. A \\end{...}",
  "or } is missing.",
  "File ended while scanning text of \\foo" },

{ "double-subscript", LatexOutputInfo::itmError, "", 'P', "Double subscript", CONF_CERTAIN,
  "Two _ on one base, as in x_1_2. Brace them: x_{1_2}.",
  "",
  "Double subscript" },

{ "double-superscript", LatexOutputInfo::itmError, "", 'P', "Double superscript", CONF_CERTAIN,
  "Two ^ on one base, as in x^1^2. Brace them: x^{1^2}.",
  "",
  "Double superscript" },

{ "missing-right", LatexOutputInfo::itmError, "", 'P', "Missing \\right. inserted", CONF_CERTAIN,
  "\\left has no matching \\right. They must pair inside one formula;",
  "use \\right. for an invisible one.",
  "Missing \\right. inserted" },

{ "extra-right", LatexOutputInfo::itmError, "", 'P', "Extra \\right", CONF_CERTAIN,
  "\\right with no \\left. Use \\left. to open invisibly.",
  "",
  "Extra \\right" },

{ "invalid-character", LatexOutputInfo::itmError, "", 'P', "Text line contains an invalid character", CONF_LIKELY,
  "A byte the input encoding cannot read. Save the file as UTF-8; on",
  "pdfLaTeX add \\usepackage[utf8]{inputenc}.",
  "Text line contains an invalid character" },

{ "capacity-exceeded", LatexOutputInfo::itmError, "", 'P', "TeX capacity exceeded", CONF_LIKELY,
  "Almost always a macro that expands into itself. Check any",
  "\\renewcommand added recently.",
  "TeX capacity exceeded, sorry [main memory size=5000000]" },

{ "emergency-stop", LatexOutputInfo::itmError, "", 'P', "Emergency stop", CONF_CERTAIN,
  "TeX gave up. The real problem is the last error reported above this.",
  "",
  "Emergency stop" },

/* --- LaTeX kernel -------------------------------------------------------- */

{ "unknown-graphics-ext", LatexOutputInfo::itmError, "", 'R', "^Unknown graphics extension: (\\.[A-Za-z0-9]+)", CONF_CERTAIN,
  "graphicx cannot read \\1 files. pdfLaTeX takes .pdf, .png and .jpg;",
  "latex with dvips takes .eps.",
  "Unknown graphics extension: .bmp" },

{ "no-bounding-box", LatexOutputInfo::itmError, "", 'S', "no BoundingBox", CONF_CERTAIN,
  "The image carries no size. State one with",
  "\\includegraphics[width=\\linewidth]{...}, or convert it to PDF.",
  "Cannot determine size of graphic (no BoundingBox)" },

{ "environment-undefined", LatexOutputInfo::itmError, "", 'R', "^Environment ([A-Za-z0-9*@]+) undefined", CONF_LIKELY,
  "No environment named \\1. Check the spelling, or load the package",
  "that defines it.",
  "Environment algorithm undefined" },

{ "begin-ended-by-end", LatexOutputInfo::itmError, "", 'R', "^\\\\begin\\{([^}]+)\\} on input line ([0-9]+) ended by \\\\end\\{([^}]+)\\}", CONF_CERTAIN,
  "\\begin{\\1} at line \\2 is closed by \\end{\\3}. One of the two names",
  "is wrong, or a nested environment was left open.",
  "\\begin{itemize} on input line 12 ended by \\end{enumerate}" },

{ "command-already-defined", LatexOutputInfo::itmError, "", 'R', "^Command (\\\\[A-Za-z@]+) already defined", CONF_CERTAIN,
  "\\1 exists already. Use \\renewcommand, or pick another name.",
  "",
  "Command \\foo already defined" },

{ "option-clash", LatexOutputInfo::itmError, "", 'R', "^Option clash for package ([A-Za-z0-9@_-]+)", CONF_CERTAIN,
  "\\1 is loaded twice with different options. Put all of its options on",
  "the first \\usepackage, or pass them through \\documentclass.",
  "Option clash for package xcolor" },

{ "usepackage-before-documentclass", LatexOutputInfo::itmError, "", 'P', "\\usepackage before \\documentclass", CONF_CERTAIN,
  "\\usepackage must come after \\documentclass. Before it, use",
  "\\RequirePackage.",
  "\\usepackage before \\documentclass" },

{ "only-in-preamble", LatexOutputInfo::itmError, "", 'S', "Can be used only in preamble", CONF_CERTAIN,
  "That command belongs before \\begin{document}.",
  "",
  "Can be used only in preamble" },

{ "missing-item", LatexOutputInfo::itmError, "", 'P', "Something's wrong--perhaps a missing \\item", CONF_LIKELY,
  "A list or table has content before its first \\item or row, or the",
  "environment is empty.",
  "Something's wrong--perhaps a missing \\item" },

{ "lonely-item", LatexOutputInfo::itmError, "", 'P', "Lonely \\item", CONF_CERTAIN,
  "\\item outside itemize, enumerate or description.",
  "",
  "Lonely \\item--perhaps a missing list environment" },

{ "no-line-to-end", LatexOutputInfo::itmError, "", 'P', "There's no line here to end", CONF_CERTAIN,
  "\\\\ on an empty line. Use \\vspace{...} or a blank line for space.",
  "",
  "There's no line here to end" },

{ "verb-ended-by-eol", LatexOutputInfo::itmError, "", 'S', "\\verb ended by end of line", CONF_CERTAIN,
  "\\verb may not span lines, nor appear in a caption, footnote or macro",
  "argument. Use \\texttt{} or the verbatim environment.",
  "\\verb ended by end of line" },

{ "illegal-array-arg", LatexOutputInfo::itmError, "", 'P', "Illegal character in array arg", CONF_CERTAIN,
  "The column spec has a letter tabular does not know. l, c, r and p{}",
  "are built in; for m, b or X load array or tabularx.",
  "Illegal character in array arg" },

{ "missing-p-arg", LatexOutputInfo::itmError, "", 'P', "Missing p-arg in array arg", CONF_CERTAIN,
  "p needs a width: p{3cm}.",
  "",
  "Missing p-arg in array arg" },

{ "not-in-outer-par-mode", LatexOutputInfo::itmError, "", 'P', "Not in outer par mode", CONF_CERTAIN,
  "A float cannot go inside a box, a list or another float. Move it out,",
  "or use \\captionof.",
  "Not in outer par mode" },

/* --- packages ------------------------------------------------------------ */

{ "biblatex-run-biber", LatexOutputInfo::itmError, "biblatex", 'R', "not created: run biber", CONF_CERTAIN,
  "Run biber on this document, not bibtex, then LaTeX twice.",
  "",
  "File 'main.bbl' not created: run biber" },

{ "beamer-overwriting-theme", LatexOutputInfo::itmError, "beamer", 'P', "Overwriting theme", CONF_CERTAIN,
  "\\usetheme was called more than once. Keep one, in the preamble.",
  "",
  "Overwriting theme: default is not allowed" },

{ "pgfkeys-unknown-key", LatexOutputInfo::itmError, "pgfkeys", 'S', "I do not know the key", CONF_LIKELY,
  "TikZ has no such option. Check the spelling, or load the library that",
  "adds it with \\usetikzlibrary{...}.",
  "I do not know the key '/tikz/important line'" },

{ "minted-shell-escape", LatexOutputInfo::itmError, "minted", 'S', "-shell-escape", CONF_CERTAIN,
  "Recompile with -shell-escape: pdflatex -shell-escape main.tex.",
  "",
  "You must invoke LaTeX with the -shell-escape flag" },

/* --- warnings ------------------------------------------------------------ */

{ "citation-undefined", LatexOutputInfo::itmWarning, "", 'R', "^Citation `([^']+)' .*undefined", CONF_CERTAIN,
  "No bibliography entry for \\1. Add it, run bibtex or biber, then",
  "LaTeX twice.",
  "Citation `knuth84' on page 1 undefined" },

{ "reference-undefined", LatexOutputInfo::itmWarning, "", 'R', "^Reference `([^']+)' .*undefined", CONF_CERTAIN,
  "No \\label{\\1} anywhere in the document. Add it or fix the key, then",
  "rerun LaTeX.",
  "Reference `sec:intro' on page 1 undefined" },

{ "label-multiply-defined", LatexOutputInfo::itmWarning, "", 'R', "^Label `([^']+)' multiply defined", CONF_CERTAIN,
  "Two \\label{\\1}. Make the keys unique; a file included twice also",
  "causes this.",
  "Label `fig:one' multiply defined" },

{ "rerun-needed", LatexOutputInfo::itmWarning, "", 'P', "Label(s) may have changed", CONF_CERTAIN,
  "Run LaTeX once more.",
  "",
  "Label(s) may have changed. Rerun to get cross-references right" },

{ "undefined-references", LatexOutputInfo::itmWarning, "", 'P', "There were undefined references", CONF_CERTAIN,
  "Rerun LaTeX. If they persist, the \\label or bibliography entry really",
  "is missing.",
  "There were undefined references" },

{ "hyperref-pdfdocencoded", LatexOutputInfo::itmWarning, "hyperref", 'S', "Token not allowed in a PDFDocEncoded string", CONF_CERTAIN,
  "A command in a heading cannot go into a PDF bookmark. Wrap it:",
  "\\texorpdfstring{$x$}{x}.",
  "Token not allowed in a PDFDocEncoded string" },

{ "hyperref-pdfpagelabels", LatexOutputInfo::itmWarning, "hyperref", 'S', "pdfpagelabels", CONF_LIKELY,
  "hyperref could not read \\thepage. Harmless unless you need PDF page",
  "labels; load hyperref after anything that changes page numbering.",
  "Option `pdfpagelabels' is turned off because \\thepage is undefined" },

{ "pgf-obsolete", LatexOutputInfo::itmWarning, "pgf", 'S', "This package is obsolete", CONF_CERTAIN,
  "Delete that \\usepackage; pgf or tikz already provides it.",
  "",
  "This package is obsolete and no longer needed" },

{ "font-shape-undefined", LatexOutputInfo::itmWarning, "", 'R', "^Font shape `([^']+)' undefined", CONF_LIKELY,
  "That family, series or shape does not exist and LaTeX substituted one.",
  "Load \\usepackage{lmodern} for a scalable family.",
  "Font shape `U/ulsy/m/n' undefined" },

{ "no-file", LatexOutputInfo::itmWarning, "", 'R', "^No file (.+)", CONF_LIKELY,
  "LaTeX will create \\1 on this run. If it is still missing after a",
  "second run, the file really is absent.",
  "No file chapter.aux" },

/* --- badboxes ------------------------------------------------------------ */

{ "overfull-hbox-output", LatexOutputInfo::itmBadBox, "", 'S', "while \\output is active", CONF_CERTAIN,
  "This happened while building the page, so there is no source line.",
  "Look at the last float, header or footer on that page.",
  "Overfull \\hbox (10.0pt too wide) has occurred while \\output is active" },

{ "overfull-hbox", LatexOutputInfo::itmBadBox, "", 'P', "Overfull \\hbox", CONF_LIKELY,
  "Text runs past the right margin. Reword it, allow a break with \\-,",
  "or load \\usepackage{microtype}.",
  "Overfull \\hbox (2.84398pt too wide) in paragraph" },

{ "underfull-hbox", LatexOutputInfo::itmBadBox, "", 'P', "Underfull \\hbox", CONF_LIKELY,
  "A line was stretched to fit, usually around a long unbreakable word",
  "or a URL. Load \\usepackage{url} for those.",
  "Underfull \\hbox (badness 10000) in paragraph" },

{ "overfull-vbox", LatexOutputInfo::itmBadBox, "", 'P', "Overfull \\vbox", CONF_LIKELY,
  "Content overflowed the page height. Shorten it, or move a float off",
  "this page.",
  "Overfull \\vbox (12.0pt too high) has occurred" },

{ "underfull-vbox", LatexOutputInfo::itmBadBox, "", 'P', "Underfull \\vbox", CONF_LIKELY,
  "A page was stretched to fill, usually a float that could not be",
  "placed nearby.",
  "Underfull \\vbox (badness 10000) has occurred" },

{ "float-too-large", LatexOutputInfo::itmWarning, "", 'R', "^Float too large for page by (\\S+)", CONF_CERTAIN,
  "The float is \\1 taller than \\textheight. Scale it, or give it its",
  "own page with [p].",
  "Float too large for page by 12.0pt" },

{ "h-float-specifier", LatexOutputInfo::itmWarning, "", 'S', "`h' float specifier changed to `ht'", CONF_CERTAIN,
  "[h] alone leaves LaTeX nowhere to put it. Write [ht], or [H] with",
  "\\usepackage{float}.",
  "`h' float specifier changed to `ht'" },

};
