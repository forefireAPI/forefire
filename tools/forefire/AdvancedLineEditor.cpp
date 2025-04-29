#include "AdvancedLineEditor.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

namespace advanced_editor {

// --- Helper functions to set/restore terminal mode ---
static void setRawMode(termios &oldt) {
    termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

static void restoreMode(const termios &oldt) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

// --- Build the command dictionary (commandMan) ---
static const std::unordered_map<std::string, std::string>& buildCommandMan() {
    static std::unordered_map<std::string, std::string> cmdMan = []() -> std::unordered_map<std::string, std::string> {
        std::unordered_map<std::string, std::string> m;
        m["FireDomain"] =
        "FireDomain[sw=(x,y,z);ne=(x,y,z);t=seconds|date=YYYY-MM-DDTHH:MM:SSZ;opt:BBoxWSEN=(8.36215875,41.711125,9.1366311,42.28667)]\n"
        "Create the main FireDomain; fronts and nodes will be created within this FireDomain\n"
        "Example: FireDomain[sw=(0,0,0);ne=(64000,64000,0);BBoxWSEN=(lonWest,latSouth,lonEast,latNorth);t=2400]\n"
        "Arguments:\n"
        " - 'sw': Southwest point (x,y,z) in meters\n"
        " - 'ne': Northeast point (x,y,z) in meters\n"
        " - 't': Time associated with the fire domain in seconds or as an absolute ISO GMT date\n"
        " - 'opt:BBoxWSEN': Optional WGS coordinates for the north-oriented bounding box (lonWest,latSouth,lonEast,latNorth)";

        m["    FireFront"] =
        "FireFront[id=<front_id>;domain=<domain_id>;t=<time>]\n"
        "Creates a FireFront within a FireDomain or another FireFront. The prefixed spacing defines its hierarchical level.\n"
        "A FireFront in a FireDomain has 4 spaces before the command; an inner FireFront adds 4 more spaces.\n"
        "Example: FireFront[id=26;domain=0;t=0]\n"
        "Arguments:\n"
        " - 'id': Identifier for the fire front\n"
        " - 'domain': Domain ID where the front is created\n"
        " - 't': Time at which the fire front is created";
    
        m["        FireNode"] =
            "FireNode[loc=(x,y,z);vel=(x,y,z);t=seconds]\n"
            "Creates a FireNode within a FireFront. The prefixed spacing defines the hierarchical level of the element.\n"
            "A FireNode inside a FireFront is defined with 4 additional spaces relative to its parent.\n"
            "Example: FireNode[domain=0;id=1;fdepth=2;kappa=0.1;loc=(3.5,2.6,1.1);vel=(-0.1,-0.03,0.01);t=2.1;state=moving;frontId=2]\n"
            "Arguments:\n"
            " - 'loc': Spatial coordinates (x,y,z) where the node is created\n"
            " - 'vel': Initial velocity (x,y,z) of the node\n"
            " - 't': Time associated with the fire node in seconds\n"
            " - 'opt:domain': Domain ID where the node is created\n"
            " - 'opt:id': Identifier for the fire node\n"
            " - 'opt:fdepth': Initial fire depth in meters\n"
            " - 'opt:kappa': Initial curvature factor (tan value)\n"
            " - 'opt:state': State of the node (init, moving, splitting, merging, final)";
        
        m["@"] =
            "@t=seconds\n"
            "Schedule operator to trigger the command at time t=seconds\n"
            "Example: print[]@t=1200\n"
            "Arguments:\n"
            " - 't': Time in seconds when the scheduled command should execute";
        
        m["startFire"] =
            "startFire[loc=(x,y,z)|lonlat=(lon,lat);t=seconds|date=YYYY-MM-DDTHH:MM:SSZ]\n"
            "Creates the smallest possible triangular fire front at the specified location and time\n"
            "Example: startFire[loc=(0.0,0.0,0.0),t=0.]\n"
            "Arguments:\n"
            " - 'loc': Starting location of the fire (coordinates x,y,z)\n"
            " - 'lonlat': WGS coordinates of the ignition point\n"
            " - 't': Time when the fire is started (in seconds or as an absolute ISO GMT date)";
        
        m["step"] =
            "step[dt=seconds]\n"
            "Advances the simulation by the specified time duration\n"
            "Example: step[dt=5.]\n"
            "Arguments:\n"
            " - 'dt': Duration (in seconds) for which the simulation will run";

        m["addLayer"] =
            "addLayer[name=]\n"
            "add a constant layer of type type (default data) with name and value V (default search for parameter of same name, then 0), optional modelName = with bounds matching the FireDomain \n"
            "Example: addLayer[name=heatFlux;type=flux;modelName=heatFluxBasic;value=3]\n";


        m["trigger"] =
            "trigger[fuelIndice=<value>;loc=(x,y,z);fuelType=<int|wind>;vel=(vx,vy,vz);t=<time>]\n"
            "Triggers a change in simulation data, such as fuel or dynamic wind conditions\n"
            "Example: trigger[fuelIndice=3;loc=(10,20,0);fuelType=1;vel=(0.5,0.5,0);t=10]\n"
            "Arguments:\n"
            " - 'fuelIndice': Fuel index value\n"
            " - 'loc': Location (x,y,z) where the trigger is applied\n"
            " - 'fuelType': Fuel type as an integer, or 'wind' for dynamic wind trigger\n"
            " - 'vel': Velocity vector (vx,vy,vz) associated with the trigger\n"
            " - 't': Time at which the trigger is activated";
        
        m["goTo"] =
            "goTo[t=seconds]\n"
            "Advances the simulation to the specified time\n"
            "Example: goTo[t=56.2]\n"
            "Arguments:\n"
            " - 't': The desired simulation time to advance to";
        
        m["print"] =
            "print[opt:filename]\n"
            "Prints a representation of the current simulation state to console or in file un the current dumpMode\n"
            "dumpMode 'ff' is the native that could be reparsed by forefire, json is a compact cartesian json, geojson and kml are projected\n"
            "Example: print[front_state.ff]\n"
            "Arguments:\n"
            " - opt: the filename for output";
        
        m["save"] =
            "save[opt:filename=landscape_file.nc;opt:fields=(altitude,wind,fuel)]]\n"
            "without argument saves the current simulation arrival time state in netcdf format with standard name ForeFire.'domainID'.nc\n"
            "Example: save[]\n"
            "Arguments:\n"
            " - opt:filename if given, landscape file is saved instead"
            " - opt:fields fields to save in landscape file in (altitude,wind,fuel)";
        
        m["loadData"] =
            "loadData[landscape_file.nc;YYYY-MM-DDTHH:MM:SSZ]\n"
            "Loads a NetCDF data file into the simulation at a certain UTC date\n"
            "Example: loadData[data.nc;2020-02-10T17:35:54Z]\n"
            "Arguments:\n"
            " - a netcdf landscape file"
            " - the UTC date";
        
        m["plot"] =
        "plot[parameter=(speed|arrival_time_of_front|fuel|altitude|slope|windU|windV|Rothermel);"
        "filename=<fname.png/jpg/nc/asc>; "
        "opt:range=(min,max); "
        "opt:area=(BBoxWSEN=(w_lon,s_lat,e_lon,n_lat) | BBoxLBRT=(leftX,bottomY,rightX,topY | active)); "
        "opt:size=(eni,enj); "
        "opt:cmap=<colormap>; "
        "opt:histbins=<integer>; "
        "opt:projectionOut=(json|<fname.kml>)]\n"
        "\n"
        "Generates a plot of the simulation parameter. The output file format is determined by the filename extension:\n"
        " - PNG/JPG: Creates an image file.\n"
        " - NC: Creates a NetCDF file containing the data matrix and latitude/longitude arrays.\n"
        " - ASC: Creates an ASCII file suitable for GIS applications.\n"
        "\n"
        "Optional projection output:\n"
        " - 'projectionOut=json': Outputs the bounding box as a JSON formatted string.\n"
        " - 'projectionOut=<fname.kml>': Saves a KML file with a GroundOverlay referencing the generated image.\n"
        "\n"
        "Example:\n"
        " plot[parameter=speed;filename=out.png;opt:range=(0,0.1);opt:area=(BBoxWSEN=(8.36215875,41.711125,9.1366311,42.28667));opt:cmap=viridis;opt:histbins=50]\n"
        "\n"
        "Arguments:\n"
        " - 'parameter': Simulation parameter to plot (speed | arrival_time_of_front | fuel | altitude | slope | windU | windV | Rothermel)\n"
        " - 'filename': Output file name. The extension (.png, .jpg, .nc, .asc) selects the output format.\n"
        " - 'opt:range': Optional data range for the plot in the format (min,max).\n"
        " - 'opt:area': Optional area specification. Use 'BBoxWSEN' for geographic coordinates (west_lon, south_lat, east_lon, north_lat) or\n"
        "               'BBoxLBRT' for Cartesian coordinates (leftX, bottomY, rightX, topY), or use 'active' for the active domain subset.\n"
        " - 'opt:size': Optional matrix size as a comma-separated pair (eni,enj). If not provided, default resolution is used.\n"
        " - 'opt:cmap': Optional colormap (e.g., viridis, turbo, fuel).\n"
        " - 'opt:histbins': Optional integer specifying the number of histogram bins between min and max.\n"
        " - 'opt:projectionOut': Optional projection output. Use 'json' for a JSON string of domain boundaries, or a filename ending in .kml\n"
        "                        to generate a KML file with a GroundOverlay referencing the output image.";
        
        m["computeSpeed"] =
            "computeSpeed\n"
            "Computes and returns an array of speed values using the first registered propagation model\n"
            "Example: computeSpeed\n"
            "Arguments:\n"
            " - Uses the active propagation model to calculate simulation speeds";
        
        m["setParameter"] =
            "setParameter[param=<value>]\n"
            "Sets a single simulation parameter to a given value\n"
            "Example: setParameter[kappa=0.05]\n"
            "Arguments:\n"
            " - 'param': Name of the parameter to set\n"
            " - 'value': Value to assign to the parameter";
        
        m["setParameters"] =
            "setParameters[param1=val1;param2=val2;...;paramn=valn]\n"
            "Sets multiple simulation parameters at once\n"
            "Example: setParameters[kappa=0.05;fdepth=20]\n"
            "Arguments:\n"
            " - Sets each parameter to its specified value; separate parameters with semicolons";
        
        m["getParameter"] =
            "getParameter[key]\n"
            "Retrieves the value of a specified simulation parameter\n"
            "Example: getParameter[dumpMode]\n"
            "Arguments:\n"
            " - 'key': The name of the parameter to retrieve";
        
        m["include"] =
            "include[input=<filename>]\n"
            "Executes commands from the specified file\n"
            "Example: include[input=commands.txt]\n"
            "Arguments:\n"
            " - 'input': Filename containing simulation commands to execute";
        
        m["clear"] =
            "clear\n"
            "Clears all simulation data\n"
            "Example: clear\n"
            "Arguments:\n"
            " - Clears the simulation data to reset the state";
        
        m["systemExec"] =
            "systemExec[command=<system_command>]\n"
            "Executes a system command\n"
            "Example: systemExec[command=ls -la]\n"
            "Arguments:\n"
            " - 'command': The system command to execute";
        
        m["listenHTTP"] =
            "listenHTTP[host=<hostname>;port=<port>]\n"
            "Launches an HTTP server to listen for simulation commands\n"
            "Example: listenHTTP[host=127.0.0.1;port=8080]\n"
            "Arguments:\n"
            " - 'host': Hostname or IP address for the server\n"
            " - 'port': Port number on which the server will listen";

        m["help"] =
            "help[]\n"
            "Displays a list of available commands and their brief descriptions.\n"
            "Use Tab-Tab on a specific command name for full details.";
        
        m["quit"] =
            "quit\n"
            "Terminates the simulation\n"
            "Example: quit\n"
            "Arguments:\n"
            " - Ends the simulation execution";
        return m;
    }();
    return cmdMan;
}

const std::unordered_map<std::string, std::string>& LineEditor::getCommandMan() {
    return buildCommandMan();
}

// --- Syntax coloring ---
// This function colors any occurrence of a recognized command (from our dictionary) in green.
std::string LineEditor::applySyntaxColoring(const std::string &line) {
    std::string result = line;
    const std::string colorOn = "\033[32m"; // green
    const std::string colorOff = "\033[0m";  // reset
    const auto &cmdMan = LineEditor::getCommandMan();
    for (const auto &pair : cmdMan) {
        size_t pos = 0;
        while ((pos = result.find(pair.first, pos)) != std::string::npos) {
            result.replace(pos, pair.first.length(), colorOn + pair.first + colorOff);
            pos += colorOn.length() + pair.first.length() + colorOff.length();
        }
    }
    return result;
}

// --- Redraw ---
// Clears the current line and reprints the prompt (in orange) plus the syntax-colored line.
// Then, moves the cursor left by (line.size() - cursorPos) positions.
void LineEditor::redraw(const std::string &prompt, const std::string &line, int cursorPos) {
    // Clear the current line.
    std::cout << "\33[2K\r";
    // Print the prompt in orange.
    std::cout << "\033[38;5;208m" << prompt << "\033[0m";
    // Print the syntax-colored line.
    std::cout << applySyntaxColoring(line);
    // Calculate how many characters from the end we need to move left.
    int visibleLength = static_cast<int>(line.size());
    int moveLeft = visibleLength - cursorPos;
    if (moveLeft > 0) {
        std::cout << "\033[" << moveLeft << "D";
    }
    std::cout.flush();
}

// --- Auto-completion ---
// This function looks for command dictionary keys starting with the current line.
bool LineEditor::processAutoCompletion(std::string &line, const std::string &prompt) {
    const auto &cmdMan = LineEditor::getCommandMan();
    std::vector<std::string> matches;
    for (const auto &pair : cmdMan) {
        if (pair.first.compare(0, line.size(), line) == 0) {
            matches.push_back(pair.first);
        }
    }
    if (matches.empty())
        return false;
    if (matches.size() == 1) {
        line = matches[0];
        return true;
    } else {
        std::cout << "\n";
        for (const auto &match : matches) {
            std::cout << "\033[32m  " << match << "\033[0m" << "\n";
        }
        std::cout << "\033[38;5;208m" << prompt << "\033[0m" << applySyntaxColoring(line);
        std::cout.flush();
        return false;
    }
}

// --- Main input function ---
// This function now maintains a cursor position (cursorPos) and supports left/right navigation.
std::string LineEditor::getLine(const std::string &prompt) {
    std::string line;
    int cursorPos = 0; // Position within the line.
    std::cout << "\033[38;5;208m" << prompt << "\033[0m";
    std::cout.flush();

    termios oldt;
    setRawMode(oldt);

    static std::vector<std::string> history;
    static int history_index = -1; // -1 means no active history navigation

    static bool lastWasTab = false;

    while (true) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n <= 0)
            break;

        if (c == '\n' || c == '\r') {
            std::cout << std::endl;
            break;
        }
        // Backspace (or DEL): delete character before cursor.
        if (c == 127 || c == '\b') {
            if (cursorPos > 0) {
                line.erase(cursorPos - 1, 1);
                cursorPos--;
            }
            redraw(prompt, line, cursorPos);
            lastWasTab = false;
            continue;
        }
        // Tab key (ASCII 9) for auto-completion.
        if (c == '\t') {
            if (lastWasTab) {
                std::string key = line;
                while (!key.empty() && key.back() == ' ')
                    key.pop_back();
                size_t pos = key.find('[');
                if (pos != std::string::npos)
                    key = key.substr(0, pos);
                const auto &cmdMan = LineEditor::getCommandMan();
                auto it = cmdMan.find(key);
                if (it != cmdMan.end()) {
                    std::cout << "\n\033[36m" << it->second << "\033[0m" << std::endl;
                    std::cout << "\033[38;5;208m" << prompt << "\033[0m" << applySyntaxColoring(line);
                    // After printing help, reposition the cursor.
                    int moveLeft = static_cast<int>(line.size()) - cursorPos;
                    if (moveLeft > 0) {
                        std::cout << "\033" << moveLeft << "D";
                    }
                    std::cout.flush();
                }
                lastWasTab = false;
            } else {
                bool updated = processAutoCompletion(line, prompt);
                if (updated) {
                    cursorPos = static_cast<int>(line.size());
                    redraw(prompt, line, cursorPos);
                }
                lastWasTab = true;
            }
            continue;
        }
        // Reset double-Tab flag if other keys are pressed.
        lastWasTab = false;
        
        // Check for escape sequences (arrow keys).
        if (c == 27) { // ESC
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 0)
                continue;
            if (read(STDIN_FILENO, &seq[1], 1) == 0)
                continue;
            if (seq[0] == '[') {
                if (seq[1] == 'A') { // Up arrow (history)
                    if (!history.empty() && history_index < static_cast<int>(history.size()) - 1) {
                        history_index++;
                        line = history[history.size() - 1 - history_index];
                        cursorPos = static_cast<int>(line.size());
                        redraw(prompt, line, cursorPos);
                    }
                    continue;
                } else if (seq[1] == 'B') { // Down arrow (history)
                    if (history_index > 0) {
                        history_index--;
                        line = history[history.size() - 1 - history_index];
                        cursorPos = static_cast<int>(line.size());
                        redraw(prompt, line, cursorPos);
                    } else if (history_index == 0) {
                        history_index = -1;
                        line.clear();
                        cursorPos = 0;
                        redraw(prompt, line, cursorPos);
                    }
                    continue;
                } else if (seq[1] == 'D') { // Left arrow
                    if (cursorPos > 0) {
                        cursorPos--;
                        redraw(prompt, line, cursorPos);
                    }
                    continue;
                } else if (seq[1] == 'C') { // Right arrow
                    if (cursorPos < static_cast<int>(line.size())) {
                        cursorPos++;
                        redraw(prompt, line, cursorPos);
                    }
                    continue;
                }
            }
        }
        // Regular character: insert at the current cursor position.
        line.insert(cursorPos, 1, c);
        cursorPos++;
        redraw(prompt, line, cursorPos);
    }

    restoreMode(oldt);

    if (!line.empty()) {
        history.push_back(line);
        history_index = -1;
    }
    return line;
}

} // namespace advanced_editor