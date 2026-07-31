/**
 * @project pplatex
 * @author  Stefan Hepp (stefan@stefant.org)
 * 
 * Copyright: 2009 Stefan hepp
 * Licence: GPL v3
 * See 'COPYRIGHT.txt' for copyright and licensing information.
 **/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>

#if defined( __WIN32__ ) || defined( _WIN32 )

#include <stdlib.h>
#define popen(x,y) _popen(x,y)
#define pclose(x) _pclose(x)
#define WEXITSTATUS(x) ((x) == -1 ? 1 : (x))

#else

#include <sys/wait.h>

#endif

#include "latexoutputfilter.h"
#include "style.h"
#include "render.h"
#include "knowledge.h"

using namespace std;

/**
 * Match a "--name=value" option and hand back the value. Options that select
 * between a fixed set of words take their value this way rather than as a
 * separate argument, so that they can never be confused with a latex option.
 */
static bool optionValue(const string& arg, const char *name, string& value)
{
    string prefix = string(name) + "=";

    if ( arg.length() <= prefix.length() || arg.compare(0, prefix.length(), prefix) != 0 ) {
	return false;
    }

    value = arg.substr(prefix.length());
    return true;
}

/**
 * Reduce argv[0] to the bare name pplatex was invoked as, so that the LaTeX
 * engine can be selected from it. Strips any directory prefix and, on Windows,
 * a trailing executable suffix.
 */
static string programName(const char* argv0) {
    string name(argv0);

    size_t pos = name.find_last_of("/\\");
    if ( pos != string::npos ) {
	name = name.substr(pos+1);
    }

    static const string exe = ".exe";
    if ( name.length() > exe.length() ) {
	string tail = name.substr(name.length() - exe.length());
	for (size_t i = 0; i < tail.length(); i++) {
	    tail[i] = tolower(tail[i]);
	}
	if ( tail == exe ) {
	    name = name.substr(0, name.length() - exe.length());
	}
    }

    return name;
}

class ArgParser {
    public:
	ArgParser(int argc, char** argv) 
	{
	    this->argc = argc;
	    this->argv = argv;
	    verbose = 0;
	    help = false;
	    version = false;
	    quiet = false;
	    nobadboxes = false;
	    format = FORMAT_AUTO;
	    color = COLOR_AUTO;
	    charset = CHARSET_ASCII;
	    paths = PATH_SHORT;
	    width = 0;
	    debugmodel = false;
	    selftest = false;

	    readEnvironment();
	    parseArguments();
	}

	int isVerbose() {
	    return verbose;
	}

	bool showUsage() {
	    return help;
	}

	bool showVersion() {
	    return version;
	}

	string getProgramName() {
	    return program;
	}

	string getSourcefile() {
	    return texfile;
	}

	string getLogfile() {
	    return logfile;
	}

	string getCmdline() {
	    return cmd;
	}

	bool doParseLogfile() {
	    return parseLogfile;
	}

	bool isQuiet() {
	    return quiet;
	}

	bool noBadBoxes() {
	    return nobadboxes;
	}

	FormatMode getFormat() {
	    return format;
	}

	ColorMode getColor() {
	    return color;
	}

	CharsetMode getCharset() {
	    return charset;
	}

	PathMode getPaths() {
	    return paths;
	}

	int getWidth() {
	    return width;
	}

	bool debugModel() {
	    return debugmodel;
	}

	bool selfTest() {
	    return selftest;
	}

    private:
	int argc;
	char** argv;

	string logfile;
	string texfile;
	string program;
	string cmd;
	bool parseLogfile;
	int verbose;
	bool help;
	bool version;
	bool quiet;
	bool nobadboxes;
	FormatMode format;
	ColorMode color;
	CharsetMode charset;
	PathMode paths;
	int width;
	bool debugmodel;
	bool selftest;

	/**
	 * Presentation defaults, for setting once in a shell profile. Command
	 * line options are parsed afterwards and win.
	 */
	void readEnvironment() {
	    const char *env;

	    if ( (env = getenv("PPLATEX_FORMAT")) != 0 ) {
		parseFormatMode(env, format);
	    }
	    if ( (env = getenv("PPLATEX_COLOR")) != 0 ) {
		parseColorMode(env, color);
	    }
	}

	/** Reject an option value we do not understand rather than guess. */
	void badValue(const string& option, const string& value) {
	    cerr << "Invalid value for '" << option << "': " << value << endl;
	    exit(2);
	}

	void parseArguments() {

	    int options = 1;
	    int cmdopt = 0;
	    int verboseopt = 0;
	    int hasInteraction = 0;
	    int inputopt = 0;
	    int quietopt = 0;
	    int bbopt = 0;
	    int formatopt = 0, coloropt = 0, charsetopt = 0, pathsopt = 0, widthopt = 0;
	    int debugopt = 0;
	    string formatval, colorval, charsetval, pathsval, widthval;

	    string name = programName(argv[0]);

	    if ( name == "ppdflatex" ) {
		program = "pdflatex";
	    } else if ( name == "ppluatex" ) {
		program = "lualatex";
	    } else if ( name == "pplatex" ) {
		program = "latex";
	    } else {
		cerr << "Invalid name for application binary." << endl;
		exit(2);
	    }

	    char** args = &argv[1];

	    for (int i = 1; i < argc; i++, args++) {
		string arg(*args);
		if ( !cmdopt && (arg == "--cmd" || arg == "-c") ) {
		    cmdopt = i+1;
		}
		else if ( options == 1 && arg == "--" ) {
		    options = i+1;
		}
		else if ( !verboseopt && arg == "-v" ) {
		    verboseopt = i;
		}
		else if ( arg.compare(0, 13, "-interaction=") == 0 ) {
		    hasInteraction = i;
		}
		else if ( options == 1 && (arg == "-h" || arg == "--help") ) {
		    help = true;
		} 
		else if ( options == 1 && (arg == "-V" || arg == "--version") ) {
		    version = true;
		}
		else if ( !inputopt && (arg == "-i" || arg == "--input" ) ) {
		    inputopt = i+1;
		}
		else if ( !quietopt && (arg == "-q" || arg == "--quiet" ) ) {
		    quietopt = i;
		}
		else if ( !bbopt && (arg == "-b" || arg == "--nobadboxes" ) ) {
		    bbopt = i;
		}
		else if ( !formatopt && optionValue(arg, "--format", formatval) ) {
		    formatopt = i;
		}
		else if ( !coloropt && optionValue(arg, "--color", colorval) ) {
		    coloropt = i;
		}
		else if ( !charsetopt && optionValue(arg, "--charset", charsetval) ) {
		    charsetopt = i;
		}
		else if ( !pathsopt && optionValue(arg, "--paths", pathsval) ) {
		    pathsopt = i;
		}
		else if ( !widthopt && optionValue(arg, "--width", widthval) ) {
		    widthopt = i;
		}
		// Undocumented: dump the parsed structure instead of rendering it.
		else if ( !debugopt && arg == "--debug-model" ) {
		    debugopt = i;
		}
		else if ( options == 1 && arg == "--self-test" ) {
		    selftest = true;
		}
	    }

	    if (help || version || selftest) {
		return;
	    }

	    if ( inputopt ) {
		if ( options == 1 ) {
		    // accept all options as pplatex options if --input is used
		    options = argc + 1;
		} else if ( inputopt <= options || cmdopt ) {
		    cerr << "Option '--input' cannot be combined with latex options" << endl;
		    exit(2);
		}
	    }

	    if ( cmdopt && cmdopt < options && cmdopt < argc ) {
		program = argv[cmdopt];
	    }

	    if ( verboseopt && verboseopt < options ) {
		verbose = 1;
	    }

	    if ( quietopt && quietopt < options ) {
		quiet = true;
	    }
	    if ( bbopt && bbopt < options ) {
		nobadboxes = true;
	    }

	    if ( formatopt && formatopt < options && !parseFormatMode(formatval, format) ) {
		badValue("--format", formatval);
	    }
	    if ( coloropt && coloropt < options && !parseColorMode(colorval, color) ) {
		badValue("--color", colorval);
	    }
	    if ( charsetopt && charsetopt < options && !parseCharsetMode(charsetval, charset) ) {
		badValue("--charset", charsetval);
	    }
	    if ( pathsopt && pathsopt < options && !parsePathMode(pathsval, paths) ) {
		badValue("--paths", pathsval);
	    }
	    if ( debugopt && debugopt < options ) {
		debugmodel = true;
	    }
	    if ( widthopt && widthopt < options ) {
		width = atoi(widthval.c_str());
		if ( width <= 0 ) {
		    badValue("--width", widthval);
		}
	    }

	    if ( inputopt && inputopt < options ) {
		parseLogfile = true;
		logfile = inputopt < argc ? argv[inputopt] : "-";
	    } 
	    else 
	    {
		parseLogfile = false;

		// build cmdline
		if ( !buildCmd(argc, argv, options, hasInteraction) ) {
		    return;
		}
	
		// get logfile name
		
		size_t fpos = texfile.find_last_of("/\\");
		if ( fpos == string::npos ) {
		    fpos = 0;
		} else {
		    fpos++;
		}

		size_t epos = texfile.length();

		if (epos > 4) {
		    string ext = texfile.substr(texfile.length()-4);
		    if ( ext == ".tex" || ext == ".TEX" ) {
			epos -= 4;
		    }
		}

		logfile = texfile.substr( fpos, epos - fpos );
	    }
	}

	int buildCmd(int argc, char** argv, int options, int hasInteraction) {

	    cmd = program;

	    if ( hasInteraction && hasInteraction < options ) {
		cerr << "Invalid option for " << program << ": -interaction" << endl;
		exit(2);
	    }
	    if ( !hasInteraction ) {
		cmd += " -interaction=nonstopmode";
	    }
	    
	    if ( options >= argc ) {
		// no options, this is odd
		cerr << "No options given for '" << program << "', exiting!" << endl;	
		exit(2);
	    }

	    char** args = &argv[options];
	    int texopt = 0;

	    for (int i = options; i < argc; i++, args++) {
		cmd += ' ';
		cmd += *args;

		if ( !texopt && *args[0] != '-' ) {
		    texopt = i;
		    texfile = *args;
		}
	    }

	    if ( !texopt ) {
		if ( help ) {
		    return 0;
		}
		cerr << "No tex-file has been given!" << endl;
		exit(3);
	    }

	    return texopt;
	}
};

static void usage(char* program) {
    cout << "Usage: " << program << " [<pplatex options> --] <latex options>" << endl;
    cout << endl;
    cout << "  pplatex options:" << endl;
    cout << "    -c, --cmd <cmd>    Execute <cmd> to compile the tex file" << endl;
    cout << "    -i, --input <file> Parse logfile <file> instead of executing latex ('-' for stdin)" << endl;
    cout << "    -b                 Do not show badbox messages" << endl;
    cout << "    -q                 Do not show warnings and badbox messages" << endl;
    cout << "    -v                 Be verbosive" << endl;
    cout << "    -V, --version      Show version info" << endl;
    cout << "    -h, --help         Show this help" << endl;
    cout << endl;
    cout << "  output options:" << endl;
    cout << "    --format=MODE      auto (default), pretty, classic" << endl;
    cout << "    --color=MODE       auto (default), always, never" << endl;
    cout << "    --charset=SET      ascii (default), unicode" << endl;
    cout << "    --paths=MODE       short (default), full" << endl;
    cout << "    --width=N          Wrap the readable output at N columns" << endl;
    cout << "    --self-test        Check the built-in hint tables and exit" << endl;
    cout << endl;
    cout << "  'auto' means the readable layout on a terminal and the classic one" << endl;
    cout << "  everywhere else, so that redirected output stays machine readable." << endl;
    cout << "  PPLATEX_FORMAT and PPLATEX_COLOR set the defaults." << endl;
    cout << endl;
    cout << "  By default, if the program is called 'pplatex', 'latex' will be executed," << endl;
    cout << "  if it is called 'ppluatex' then 'lualatex' will be executed," << endl;
    cout << "  else 'pdflatex' will be used." << endl;
}

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

static void version() {
    cout << STRINGIFY(RELEASE_NAME) << ": " << STRINGIFY(RELEASE_VERSION) << endl;
    cout << "Copyright (C) 2003 Jeroen Wijnhout (wijnhout@science.uva.nl)" << endl;
    cout << "              2008 by Michel Ludwig (michel.ludwig@kdemail.net)" << endl;
    cout << "              2009,2010 Stefan Hepp (stefan@stefant.org)" << endl;

}

int main(int argc, char** argv) {

    if ( argc == 1 ) {
	cout << "No arguments supplied. Try '" << argv[0] << " -h'." << endl;
	return 1;
    }

    ArgParser parser(argc, argv);

    if ( parser.showUsage() ) {
	usage(argv[0]);
	return 0;
    }

    if ( parser.showVersion() ) {
	version();
	return 0;
    }

    if ( parser.selfTest() ) {
	return selfTestKnowledge() == 0 ? 0 : 1;
    }

    FILE *fp;
    
    if ( parser.doParseLogfile() ) {
	if ( parser.isVerbose() ) {
	    cout << "Parsing ";
	    if ( parser.getLogfile() == "-" ) {
		cout << "stdin" << endl;
	    } else {
		cout << "logfile " << parser.getLogfile() << endl;
	    }
	}

	if ( parser.getLogfile() == "-" ) {
	    fp = stdin;
	} else {
#ifdef _WIN32
	    errno_t err = fopen_s(&fp, parser.getLogfile().c_str(), "r");
	    if (err != 0) {
		fp = 0;
	    }
#else
	    fp = fopen(parser.getLogfile().c_str(), "r");
#endif
	}

	if ( !fp ) {
	    perror("Error");
	    cerr << "Unable to open ";
	    if ( parser.getLogfile() == "-" ) {
		cerr << "stdin" << endl;
	    } else {
		cerr << "logfile " << parser.getLogfile() << endl;
	    }
	    return 1;
	}
    } else {
	if ( parser.isVerbose() ) {
	    cout << "Executing: " << parser.getCmdline() << endl;
	}

	fp = popen(parser.getCmdline().c_str(), "r");

	if ( !fp ) {
	    perror("Error");
	    cerr << "Unable to execute '" << parser.getProgramName() << "'!" << endl;
	    return 1;
	}
    }

    LatexOutputFilter of(parser.getSourcefile(), parser.isVerbose(), parser.noBadBoxes() || parser.isQuiet(), parser.isQuiet());

    bool pretty = wantPretty(parser.getFormat(), stdout);

    RenderOpts opts;
    opts.width   = terminalWidth(parser.getWidth(), stdout);
    opts.color   = wantColor(parser.getColor(), stdout);
    opts.unicode = parser.getCharset() == CHARSET_UNICODE;
    opts.paths   = parser.getPaths();

    of.setDebugModel(parser.debugModel());
    of.setPretty(pretty, opts);

    of.run(fp);

    int errors, warnings, badboxes;
    of.getErrorCount( &errors, &warnings, &badboxes );

    if ( pretty ) {
	cout << renderSummary(errors, warnings, badboxes, opts);
    } else {
	cout << "Result: o) Errors: " << errors << ", Warnings: " << warnings << ", BadBoxes: " << badboxes << endl;
    }

    int status;

    if ( parser.doParseLogfile() ) {
	fclose(fp);
	status = (errors > 0) ? 1 : 0;
	cout << endl;
    } else {
        int ret = pclose(fp);
	status = WEXITSTATUS(ret);

	if ( status ) {
	    cout << "        o) " << parser.getProgramName() << " returned an error!" << endl << endl;
	} else {
	    cout << "        o) " << parser.getProgramName() << " was successful!" << endl << endl;
	}
    }

    return status;
}

