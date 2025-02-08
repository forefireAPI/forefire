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
    cout << "usage: " << name << " [-i file] [-o file]" << endl;
    cout << " -i: input command file" << endl;
    cout << " -o: output command file" << endl;
}

int ForeFire::startShell(int argc, char* argv[]) {
    ifstream* inputStream = nullptr;
    int fileopt;
    int EOO = -1;
    while ((fileopt = getopt(argc, argv, "w:i:o:v")) != EOO) {
        if (fileopt == 'v') {
            cout << ff_version << endl;
            return 1;
        }
        if (fileopt == 'w') {
            FFWebShell(optarg);
            return 1;
        } else if (fileopt == 'i') {
            inputStream = new ifstream(optarg);
            if (!inputStream) {
                cout << "wrong input file, check your settings..." << optarg << endl;
            }
        }
    }
    FFShell(inputStream);
    delete inputStream;
    return 1;
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
        cout << "Wildfire solver ForeFire Version :" << ff_version << endl;
        cout << "Copyright (C) 2025 CNRS/Univ.Corsica " << endl;
        cout << "Comes with ABSOLUTELY NO WARRANTY " << endl;
        cout << "Type 'help[]' for commands" << endl;
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