#include "../../src/include/Futils.h"
#include "../../src/include/Version.h"
#include "../../src/Command.h"
#include "AdvancedLineEditor.hpp"  // include the advanced editor

// needed for the fork
#include <sys/types.h>
#include <unistd.h>

using namespace std;

namespace libforefire {

class ForeFire {
    Command executor; // object containing all the ForeFire data

public:
    ForeFire();
    virtual ~ForeFire();

    void usage(const char*);
    int startShell(int, char*[]);
    void FFShell(ifstream* = 0);
    void FFWebShell(char*);

private:
    void executeCommandFile(char*);
    void emptyFile(char*);
    void copyFileInEndOfFile(char*, const char*);
};

ForeFire::ForeFire() : executor() {}
ForeFire::~ForeFire() {}

void ForeFire::usage(const char* name) {
    cout << "usage: " << name << " [-i file] [-l] [-v] [-w file]" << endl;
    cout << " -i file: input command file" << endl;
    cout << " -l     : Start directly in listenHTTP mode" << endl;
    cout << " -v     : print version" << endl;
    cout << " -w file: web shell mode" << endl;

}

int ForeFire::startShell(int argc, char* argv[]) {
    ifstream* inputStream = nullptr;
    int fileopt;
    int EOO = -1;
    bool startListenHttpMode = false;
    while ((fileopt = getopt(argc, argv, "w:i:o:vl")) != EOO) {
        switch (fileopt) {
            case 'l':
                startListenHttpMode = true;
                goto end_option_parsing;

            case 'v': {
                string describe = FF_GIT_DESCRIBE;
                cout << ff_version;
                if (!describe.empty()) cout << " (" << describe << ")";
                cout << endl;
                return 1;
            }
            case 'w':
                FFWebShell(optarg);
                return 1;
            case 'i':
                inputStream = new ifstream(optarg);
                if (!inputStream || !inputStream->is_open()) {
                    cerr << "Error: Could not open input file: " << optarg << endl;
                    delete inputStream;
                    return 1;
                }
                break;
            case '?': // Handle unknown options or missing args for options requiring one
            default:
                usage(argv[0]);
                return 1;
        }
    }

    end_option_parsing:;

    if (startListenHttpMode) {

        cout << "" << endl;

        string standAlone = "setParameter[runmode=standalone]";
        executor.ExecuteCommand(standAlone);

        string listenCommand = "listenHTTP[]";
        executor.ExecuteCommand(listenCommand);

        // Keep the main thread alive while the server runs. `quit[]` used to
        // end the process from inside the library; now it asks, and this is
        // the loop that answers, so a served quit[] still stops the server.
        cout << "(Press Ctrl+C to exit)" << endl;
        while (!Command::quitRequested()) {
            sleep(1);
        }

        return 0;

    } else {

        FFShell(inputStream);
        delete inputStream;
        return 0; // Normal exit for non-server mode
    }
}

void ForeFire::FFWebShell(char* path) {
    executeCommandFile(path);
    SimulationParameters *simParam = SimulationParameters::GetInstance();
    string logFile(simParam->getParameter("caseDirectory") + "/" 
                   + simParam->getParameter("ForeFireDataDirectory") + "/log.ff");
    cout << endl << "Log file : " << logFile << endl;
    copyFileInEndOfFile(path, logFile.c_str());
    emptyFile(path);
    while (true) {
        executeCommandFile(path);
        copyFileInEndOfFile(path, logFile.c_str());
        emptyFile(path);
        sleep(1);
    }
}



void ForeFire::FFShell(ifstream* inputStream) {
    if (inputStream) {
        string line;
        string standAlone = "setParameter[runmode=standalone]";
        executor.ExecuteCommand(standAlone);
        executor.executeLoop(inputStream);

    } else {

        // Colours only on a terminal, like install-forefire.sh; NO_COLOR turns them off.
        const char* term = getenv("TERM");
        bool colour = isatty(STDOUT_FILENO) && !getenv("NO_COLOR")
                      && term && string(term) != "dumb";
        auto fg = [colour](int c) { return colour ? "\033[38;5;" + to_string(c) + "m" : string(); };
        const string bold  = colour ? "\033[1m" : "";
        const string dim   = colour ? "\033[2m" : "";
        const string reset = colour ? "\033[0m" : "";

        // The FF burning from yellow to red, with a few sparks above.
        cout << endl;
        cout << fg(214) << dim << "               .    *     ." << reset << endl;
        cout << fg(208) << dim << "            *    .    *      ." << reset << endl;
        cout << fg(226) << "            ██████╗ ██████╗" << reset << endl;
        cout << fg(220) << "            ██╔═══╝ ██╔═══╝" << reset << endl;
        cout << fg(214) << "            ██████╗ ██████╗" << reset << endl;
        cout << fg(208) << "            ██╔═══╝ ██╔═══╝" << reset << endl;
        cout << fg(202) << "            ██║     ██║" << reset << endl;
        cout << fg(196) << "            ╚═╝     ╚═╝" << reset << endl;
        cout << endl;
        cout << "  " << bold << "ForeFire" << reset << " - Wildfire Propagation Simulator  "
             << fg(208) << ff_version << reset << endl;
        cout << dim << "  Copyright (C) 2026 CNRS/Univ.Corsica - see LICENSE (ABSOLUTELY NO WARRANTY)" << reset << endl;
        cout << endl;
        cout << "  " << fg(208) << "Tab" << reset << "         list all commands, or complete the one you're typing" << endl;
        cout << "  " << fg(208) << "Tab Tab" << reset << "     detailed help for a command" << endl;
        cout << "  " << fg(208) << "quit" << reset << "        leave the shell" << endl;
        cout << endl;

        std::string line;
        while (true) {
            line = advanced_editor::LineEditor::getLine("forefire> ");
            if (line == "quit" || line == "exit")
                break;
            if (line.empty())
                continue;
            executor.ExecuteCommand(line);
            // `quit[]` no longer ends the process from inside the library, so
            // the shell is what leaves on request.
            if (Command::quitRequested())
                break;
        }
    }
}

void ForeFire::executeCommandFile(char* path) {
    string command("include[" + string(path) + "]");
    fstream *src = new fstream(path, ofstream::in);
    executor.ExecuteCommand(command);
    src->close();
    delete src;
}

void ForeFire::emptyFile(char* path) {
    fstream *src = new fstream(path, ofstream::out | ofstream::trunc);
    src->close();
    delete src;
}

void ForeFire::copyFileInEndOfFile(char* srcPath, const char* dstPath) {
    ofstream *dst = new ofstream(dstPath, fstream::out | fstream::app);
    fstream *src = new fstream(srcPath, ofstream::in);
    (*dst) << src->rdbuf();
    dst->close();
    src->close();
    delete dst;
    delete src;
}

} // namespace libforefire

int main(int argc, char* argv[]) {
    using namespace libforefire;
    ForeFire FFShell;
    FFShell.startShell(argc, argv);
    return 0;
}