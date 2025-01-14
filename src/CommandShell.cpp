/*

  Copyright (C) 2012 ForeFire Team, SPE, CNRS/Universita di Corsica.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 US

*/

#include "CommandShell.h"
#include "include/Version.h"

using namespace std;

namespace libforefire {

CommandShell::CommandShell() : executor() {
}
CommandShell::~CommandShell() {
}

void CommandShell::usage(const char* name){
	cout << "usage: "  << name << " [-i file] [-o file]" << endl;
	cout << " -i: input command file" << endl;
	cout << " -o: output command file" << endl;
}

int CommandShell::startShell(int argc, char* argv[]){

	// temporary streams
	ifstream* inputStream = 0;

	// Reading the options
	int fileopt;
	int EOO = -1;
	while((fileopt = getopt(argc, argv, "w:i:o:v")) != EOO) {
		if (fileopt == 'v'){
			cout << ff_version << endl;
			return 1;
		}
		if (fileopt == 'w'){
			// Web Mode
			FFWebShell(optarg);
			return 1;
		}
		else if (fileopt == 'i'){
			// Standard Mode
			inputStream = new ifstream(optarg);
			if ( !inputStream ){
				cout << "wrong input file, check your settings..." << optarg << endl;
			}
		}
	}

	FFShell(inputStream);
	delete inputStream;
	return 1;
}

void CommandShell::FFWebShell(char* path)
{
    executeCommandFile(path);
    
    SimulationParameters *simParam = SimulationParameters::GetInstance();
    string logFile(simParam->getParameter("caseDirectory") + "/" 
                 + simParam->getParameter("ForeFireDataDirectory") + "/log.ff");

    cout << endl << "Log file : " << logFile << endl;
    
    copyFileInEndOfFile(path, logFile.c_str());
    emptyFile(path);

    while (true)
    {
        executeCommandFile(path);
        copyFileInEndOfFile(path, logFile.c_str());
        emptyFile(path);

        sleep(1);
    }
}

void CommandShell::FFShell(ifstream* inputStream){
    
	if ( inputStream ){
		// reading all the commands (one command per line) of the input file
		string line;
		//size_t numLine = 0;
		string standAlone = "setParameter[runmode=standalone]";
		executor.ExecuteCommand(standAlone);
		while ( getline( *inputStream, line ) ) {
			//numLine++;
			// checking for comments or newline
			if((line[0] == '#')||(line[0] == '*')||(line[0] == '\n'))
				continue;
			// treating the command of the current line
			executor.ExecuteCommand(line);
		}
	} else {
		cout << "Wildfire solver ForeFire Version :" << ff_version << endl;
		cout << "Copyright (C) 2025 CNRS/Univ.Corsica "<< endl;
		cout << "Comes with ABSOLUTELY NO WARRANTY " << endl;
		cout << "Type 'help[]' for commands" << endl;
		// reading all the commands (one command per line) from the terminal
		string line;
		//size_t numLine = 0;
		while ( getline( cin, line ) ) {
			//numLine++;
			// checking for comments or newline
			if((line[0] == '#')||(line[0] == '*')||(line[0] == '\n'))
				continue;
			// treating the command of the current line
			executor.ExecuteCommand(line);
		}
    }
}

void CommandShell::executeCommandFile(char* path)
{
    string command("include["+string(path)+"]");
    fstream *src = new fstream(path, ofstream::in);
    executor.ExecuteCommand(command);
    src->close();
    delete src;
}

void CommandShell::emptyFile(char* path)
{
    fstream *src = new fstream(path, ofstream::out | ofstream::trunc);
    src->close();
    delete src;
}

void CommandShell::copyFileInEndOfFile(char* srcPath, const char* dstPath)
{
    ofstream *dst = new ofstream(dstPath, fstream::out | fstream::app);
    fstream *src = new fstream(srcPath, ofstream::in);
    
    (*dst) << src->rdbuf();
    dst->close();
    src->close();
    
    delete dst;
    delete src;
}

}

int main( int argc, char* argv[] ){
	using namespace libforefire;
	CommandShell FFShell;
	FFShell.startShell(argc, argv);
	return 0;
}

