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

            case 'v':
                cout << ff_version << endl;
                return 1;
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

        // Keep the main thread alive indefinitely
        cout << "(Press Ctrl+C to exit)" << endl;
        while (true) {
            sleep(3600);
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
        while (getline(*inputStream, line)) {
            if ((line[0] == '#') || (line[0] == '*') || (line[0] == '\n'))
                continue;
            executor.ExecuteCommand(line);
        }
    } else {

        const char* logoFF = R"(
            ██████╗ ██████╗
            ██╔═══╝ ██╔═══╝
            ██████╗ ██████╗
            ██╔═══╝ ██╔═══╝
            ██║     ██║
            ╚═╝     ╚═╝
        )";
        
        cout << logoFF << endl;
        cout << "  =================================================" << endl;
        cout << "      ForeFire - Wildfire Propagation Simulator" << endl;
        cout << "  =================================================" << endl;
        cout << "  Version:   " << ff_version << endl;
        cout << "  License:   See LICENSE file (ABSOLUTELY NO WARRANTY)" << endl;
        cout << "  Copyright (C) 2025 CNRS/Univ.Corsica" << endl;
        cout << endl;
        cout << "  Using the Interactive Shell:" << endl;
        cout << "  - Press Tab to:" << endl;
        cout << "      - List all available commands (if the line is empty)." << endl;
        cout << "      - Auto-complete a command you're typing." << endl;
        cout << "  - Press Tab a second time on a command name for its detailed help." << endl;
        cout << endl;

        std::string line;
        while (true) {
            line = advanced_editor::LineEditor::getLine("forefire> ");
            if (line == "quit" || line == "exit")
                break;
            if (line.empty())
                continue;
            executor.ExecuteCommand(line);
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