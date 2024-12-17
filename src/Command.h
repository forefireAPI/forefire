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

#ifndef COMMAND_H_
#define COMMAND_H_

#include <getopt.h>
#include "ForeFireAtom.h"
#include "SimulationParameters.h"
#include "FireDomain.h"
#include "StringRepresentation.h"
#include "TimeTable.h"
#include "Simulator.h"
#include "FireFront.h"
#include "FireNode.h"
#include "include/Futils.h"
#include "EventCommand.h"
#include <vector>
#include <algorithm>

#ifndef COMMAND_DEBUG
#define COMMAND_DEBUG 1<<8

#endif




namespace libforefire {

/*! \class Command
 * \brief Commands for driving a ForeFire simulation (singleton class)
 *
 *  Command is a singleton class that
 *  defines the entry points for user-defined actions
 *  thus enabling driving a ForeFire simulation.
 */

class Command {

	/*! \class BadOption
	 * \brief dummy class for exception handling
	 * in case of a bad option inside the command */
	class BadOption {};
	/*! \class MissingTime
	 * \brief dummy class for exception handling
	 * in case of missing time inside the command */
	class MissingTime {};
	/*! \class MissingOption
	 * \brief dummy class for exception handling
	 * in case of a missing option inside the command */
	class MissingOption {
	public:
		size_t num;
		MissingOption(size_t n):num(n){}
	};

	static Command value; /*!< value of the singleton */

	Command&	operator=(Command&); // Disallowed
	Command(const Command&); // Disallowed


	static size_t currentLevel; /*!< current level of the command */

	// Definition of the command map alias
	typedef int (*cmd)(const string&, size_t&);
	typedef map<string,cmd> commandMap;  /*!< map of aliases between strings and functions to be called */
	static const int numberCommands = 21; /*!< number of possible commands */
	static commandMap makeCmds(){
		// Construction of the command translator
		commandMap trans;
		trans["FireDomain"] = &createDomain;
		trans["FireNode"] = &addFireNode;
		trans["FireFront"] = &createFireFront;
		trans["startFire"] = &startFire;
		trans["step"] = &stepSimulation;
		trans["goTo"] = &goTo;
		trans["print"] = &printSimulation;
		trans["save"] = &saveSimulation;
		trans["plot"] = &plotSimulation;
		trans["computeSpeed"] = &computeModelSpeed;
		trans["load"] = &loadSimulation;
		trans["setParameter"] = &setParameter;
		trans["setParameters"] = &setParameters;
		trans["getParameter"] = &getParameter;
		trans["trigger"] = &triggerValue;
		trans["include"] = &include;
		trans["help"] = &help;
		trans["man"] = &man;
		trans["loadData"] = &loadData;
		trans["systemExec"] = &systemExec;
		trans["clear"] = &clear;
		trans["quit"] = &quit;

		return trans;
	}
	/* A map of the commands to their effective functions */
	static const commandMap translator;  /*!< map of aliases between strings and functions to be called */

	// Definition of the command man alias
	typedef map<string,string> commandMan;
	static commandMan makeMan(){
		// Construction of the man pages
		commandMan cman;
		ostringstream mantmp;
		mantmp << "FireDomain[Psw;Pne;t;bmapdx;bmapdy]" << endl;
		mantmp << " - 'Psw' and 'Pne' are the southwest and northwest points of the boundaries" << endl;
		mantmp << "    and should be affected as in 'Psw=(0.,0.,0.)';" << endl;
		mantmp << " - 't' is the time associated with fire domain ('t=0.');" << endl;
		mantmp << " - 'bmapdx' and 'bmapdy' are the resolution of the burning map in each" << endl;
		mantmp << "    direction and should be affected as in 'bmpadx=0.1';" << endl;
		cman["FireDomain"] = mantmp.str();
		mantmp << "FireNode[loc;vel;t]" << endl;
		mantmp << " - 'loc' is the spatial point where the firenode is created ('loc=(0.,0.,0.)');" << endl;
		mantmp << " - 'vel' is the initial velocity of the firenode ('vel=(0.,0.,0.)');" << endl;
		mantmp << " - 't' is the time associated with fire node ('t=0.');" << endl;
		cman["FireNode"] = mantmp.str();
		cman["FireFront"] = "FireFront[]\n create a fire front with the following fire nodes\n";
		cman["startFire"] = "startFire[loc=(0.,0.,0.),t=0.]\n create a triangular fire front with 3 nodes around the location at time t\n";
		cman["step"] = "step[dt]\n - 'dt' is the duration for which the simulation will run ('dt=5.')\n";
		cman["goTo"] = "goTo[t]\n - 't' is the desired time till which the simulation will run ('t=56.2')\n";
		cman["print[output]"] = "print\n prints a representation of the simulation in the file 'output'\n";
		cman["save"] = "save\n saves the simulation in hdf format\n";
		cman["load"] = "loads the simulation arrival_times in netcdf format";
		cman["plot"] = "generates a png or jpg of the simulation \n parameter=speed or arrival_time_of_front;filename=outfname.png/jpg;opt:range=(0,0.1);opt:cmap=viridis;opt:histogram=true";
		cman["computeSpeed"] = "uses the first registered propp model to activate and get result from an array of values separated by ;";
		cman["setParameter[param=value]"] = "setParameter\n - sets parameter 'param' to the given 'value'";
		cman["setParameters[param1=val1;param2=val2;...;paramn=valn]"] = "setParameters\n - sets a given list of parameters to the desired values";
		cman["getParameter[key=value]"] = "gets parameter 'key' ";
		cman["include[input]"] = "include\n - executes the commands in the file 'input'";
		cman["help"] = "help\n displays messages about the usage of commands\n";
		cman["loadData"] = "loadData\n load a NC data file\n";
		cman["clear"] = "clear\n clear all the simulation data\n";
		cman["systemExec"] = "systemExec\n run a system command\n";
		cman["quit"] = "quit\n terminates the simulation\n";
		return cman;
	}
	/* A map of the commands to their man page */
	static const commandMan manpages; /*!< manual pages for the commands */

	// Definition of the dictionary structure
	struct CmdDictEntry{
		string inlineCmd;
		string usage;
		CmdDictEntry(){}
		CmdDictEntry(string s1, string s2){
			this->inlineCmd = s1;
			this->usage = s2;
		}
		CmdDictEntry(const CmdDictEntry& cdt) : \
				inlineCmd(cdt.inlineCmd)\
				, usage(cdt.usage) {}
	};
	// Construction of the dictionary of commands
	static CmdDictEntry cmdDict[numberCommands];

	// Boolean for parallel simulation
	static bool parallel;

	// Boolean for initialization
	static bool init;
	static bool currentFrontCompleted;

	// Time interval of a step of the simulation
	static double startTime;
	static double endTime;

	enum Status {
		normal = 0,	error = 1
	};

	static bool firstCommand;
	static size_t refTabs;

	static FFPoint* lastReadLoc;
	static FireNode* previousNode;
	static FireNode* leftLinkNode;
	static FireNode* rightLinkNode;

	static double bmapOutputUpdate;
	static int numBmapOutputs;
	static int numAtmoIterations;

	/*! \brief command to create the desired fire domain */
	static int createDomain(const string&, size_t&);
	/*! \brief command to create a firenode */
	static int addFireNode(const string&, size_t&);
	/*! \brief command to create a firefront from a location point*/
	static int startFire(const string&, size_t&);
	/*! \brief command to create a firefront */
	static int createFireFront(const string&, size_t&);
	/*! \brief command to run the simulation for the desired amount of time */
	static int stepSimulation(const string&, size_t&);
	/*! \brief command to run the simulation till the desired time */
	static int goTo(const string&, size_t&);
	/*! \brief command to save in print format the simulation */
	static int printSimulation(const string&, size_t&);
	/*! \brief command to save in print format the simulation */
	static int saveSimulation(const string&, size_t&);
	/*! \brief command to load in print format the simulation */
	static int loadSimulation(const string&, size_t&);
	/*! \brief command to plot in png/jpg format the simulation */
	static int plotSimulation(const string&, size_t&);
	/*! \brief command to get speed and cout a double */
	static int computeModelSpeed(const string&, size_t&);
	/*! \brief command to set a given parameter */
	static int setParameter(const string&, size_t&);
	/*! \brief command to set a given list of parameters */
	static int setParameters(const string&, size_t&);
	/*! \brief command to get a given parameters */
	static int getParameter(const string&, size_t&);
	/*! \brief command to include a file */
	static int triggerValue(const string&, size_t&);
	/*! \brief command to trigger values that will modifie runtime model parameterisation */
	static int include(const string&, size_t&);
	/*! \brief help */
	static int help(const string&, size_t&);
	/*! \brief invoking the manual pages */
	static int man(const string&, size_t&);
	/*! \brief command to load a NC data file */
	static int loadData(const string&, size_t&);
	/*! \brief command to clear the simulation */
	static int systemExec(const string&, size_t&);
	/*! \brief command to run a system trough pipe */
	static int clear(const string&, size_t&);
	/*! \brief command to quit the ForeFire shell */
	static int quit(const string&, size_t&);

	/*! \brief splits the command into the desired options */
	static void tokenize(const string&, vector<string>&, const string&);
	/*! \brief reads the value of the desired string */
	static string getString(string, string);
	/*! \brief reads the value of the desired int */
	static int getInt(string, string);
	/*! \brief reads the value of the desired double */
	static double getFloat(string, string);
	/*! \brief reads the value of the desired FFPoint */
	static FFPoint getPoint(string, string);
	/*! \brief reads the value of the desired FFVector */
	static FFVector getVector(string, string);
	/*! \brief counts the arguments in the commands */
	static size_t argCount(string);
	/*! \brief counting the tabs in the commands */
	static size_t tabsCount(string);
	/*! \brief remove the tabs in the commands */
	static string removeTabs(string);

    static void writeImage(const char* filename, const std::vector<std::vector<double>>& matrix,
                           double forced_min_val = std::numeric_limits<double>::quiet_NaN(),
                           double forced_max_val = std::numeric_limits<double>::quiet_NaN(),
                           const std::string& colormap = "grayscale"); // Default to grayscale if no colormap is specified.


    static void writeHistogram(const char* filename, const std::vector<std::vector<double>>& matrix,   int bins = 100,                        
							double forced_min_val = std::numeric_limits<double>::quiet_NaN(),
                            double forced_max_val = std::numeric_limits<double>::quiet_NaN(),
							
							const std::string& colormap = "grayscale") ;

    static void parseColorMap(const std::string& , std::vector<std::array<unsigned char, 4>>& ) ;

	static const string stringError;
	static const FFPoint pointError;
	static const FFVector vectorError;

public:

	// Reference time
	static double refTime;

	// Definition of the structure 'Session'
	struct Session{
		SimulationParameters* params;
		FireDomain* fd;
		FireDomain* fdp;
		FireFront* ff;
		StringRepresentation* outStrRep;
		StringRepresentation* outStrRepp;
		TimeTable* tt;
		Simulator* sim;
		ostream* outStream;
		int debugMode;
	};

	// Definition of the current session, on which the commands acts
	static Session currentSession; /*!< session containing all the data of the ForeFire simulation */
	//static FireDomain* domain; /*!< pointer to the current fire domain */ 
	//static void setDomain(FireDomain*); /*!< pointer to the current fire domain */

	// Vector of outputs directories
	static vector<string> outputDirs; /*!< vector of outputs directories */

	/*! \brief Default constructor */
	Command();

	/*! \brief getter of the singleton */
	static Command& instance() { return value; }
	/*! \brief Destructor */
	virtual ~Command();

	/*! \brief setting the reference time */
	static void setReferenceTime(const double&);

	/*! \brief setting the start time of the step of simulation */
	static void setStartTime(const double&);

	/*! \brief getting the current time of simulation */
	static double getTime();

	/*! \brief managing the level */
	static void increaseLevel();
	static void decreaseLevel();

	/*! \brief accessor to the domain */
	static FireDomain* getDomain();
	/*! \brief command to redirect the ostringstream */
	static void setOstringstream(ostringstream*);

	/*! \brief execute the desired command */
	static void ExecuteCommand(string&);

	/*! \brief complete the last front */
	static void completeFront(FireFront*);

	/*! \brief backup of the simulation */
	static string dumpString();
	
	

};

}

#endif /* COMMAND_H_ */
