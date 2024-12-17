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

#ifndef COMMANDSHELL_H_
#define COMMANDSHELL_H_

#include "include/Futils.h"
#include "Command.h"

// needed for the fork
#include <sys/types.h>
#include <unistd.h>

namespace libforefire {

/*! \class CommandShell
 * \brief Shell for driving a ForeFire simulation
 *
 *  CommandShell enables terminal-based driving of ForeFire simulation
 */
class CommandShell {
	Command executor; /*!< object containing all the ForeFire data */

public:
	/*! \brief Default constructor */
	CommandShell();
	/*! \brief Destructor */
	virtual ~CommandShell();

	/*! \brief displays a condensed manual page */
	void usage(const char*);
	/*! \brief starts the shell */
	int startShell(int, char*[]);
	/*! \brief ForeFire Shell */
	void FFShell(ifstream* = 0);
    /*! \brief ForeFire Web Shell */
    void FFWebShell(char*);
    
private:
    void executeCommandFile(char*);
    void emptyFile(char*);
    void copyFileInEndOfFile(char*, const char*);
};

}

#endif /* COMMANDSHELL_H_ */
