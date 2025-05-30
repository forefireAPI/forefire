def generate_radiosounding_section(year, month, day, time, kind, ground_height, ground_pressure,
                                   ground_temperature, ground_moisture, num_wind_levels, wind_levels,
                                   num_mass_levels, mass_levels):
    """
    Generate the radiosounding section for a Fortran namelist.

    :param year, month, day, time: Date and time data for the radiosounding
    :param kind: Type of radiosounding data, affects how other data is interpreted
    :param ground_height: Ground level height
    :param ground_pressure: Pressure at ground level
    :param ground_temperature: Temperature at ground level
    :param ground_moisture: Moisture at ground level
    :param num_wind_levels: Number of levels with wind data
    :param wind_levels: List of tuples with wind level data (altitude, wind1, wind2)
    :param num_mass_levels: Number of levels with mass data (including ground level)
    :param mass_levels: List of tuples with mass level data (altitude, temperature, moisture, [cloud_vars])
    :returns: String formatted as a section of a Fortran namelist
    """
    rsou = f"RSOU\n{year} {month} {day} {time:.1f}\n'{kind}'\n"
    rsou += f"{ground_height:.1f}\n{ground_pressure:.1f}\n{ground_temperature:.1f}\n{ground_moisture}\n"
    rsou += f"{num_wind_levels}\n"
    for level in wind_levels:
        rsou += " ".join(f"{x:.1f}" for x in level) + "\n"
    rsou += f"{num_mass_levels}\n"
    for level in mass_levels:
        rsou += " ".join(f"{x:.1f}" if isinstance(x, float) else str(x) for x in level) + "\n"
    
    return rsou

# Example usage of the function
example_wind_levels = [
    (85000, 20, 10),
    (70000, 30, 10)
]

example_mass_levels = [
    (90000, 280, 275),
    (60000, 271, 269)
]

print(generate_radiosounding_section(
    year=1990, month=10, day=3, time=72000, kind='STANDARD',
    ground_height=200, ground_pressure=100240, ground_temperature=287.5, ground_moisture=276,
    num_wind_levels=2, wind_levels=example_wind_levels,
    num_mass_levels=3, mass_levels=example_mass_levels
))


def generate_cstn_section(year=1994, month=4, day=22, time=36000.0,
                          num_levels=1, theta_v_ground=300.0, pressure_ground=100000.0,
                          heights=[], zonal_winds=[], meridian_winds=[], humidities=[], brunt_vaisala=[]):
    """
    Generate the CSTN section for a Fortran namelist.

    :param year: Year of the profile (integer)
    :param month: Month of the profile (integer)
    :param day: Day of the profile (integer)
    :param time: Time in seconds (real)
    :param num_levels: Number of vertical levels (integer)
    :param theta_v_ground: Virtual potential temperature at ground level (real, in Kelvin)
    :param pressure_ground: Pressure at ground level (real, in Pascal)
    :param heights: List of heights at each level (list of reals)
    :param zonal_winds: List of zonal wind components at each level (list of reals)
    :param meridian_winds: List of meridian wind components at each level (list of reals)
    :param humidities: List of relative humidity values at each level (list of reals)
    :param brunt_vaisala: List of moist Brunt Vaisala frequencies between layers (list of reals)
    """
    sections = [f"CSTN\n{year} {month} {day} {time:.1f}\n{num_levels}\n{theta_v_ground:.1f}\n{pressure_ground:.1f}"]
    
    if len(heights) > 0:
        sections.append(" ".join(f"{h:.1f}" for h in heights))
    if len(zonal_winds) > 0:
        sections.append(" ".join(f"{w:.1f}" for w in zonal_winds))
    if len(meridian_winds) > 0:
        sections.append(" ".join(f"{w:.1f}" for w in meridian_winds))
    if len(humidities) > 0:
        sections.append(" ".join(f"{h:.1f}" for h in humidities))
    if len(brunt_vaisala) > 0:
        sections.append(" ".join(f"{b:.3f}" for b in brunt_vaisala))
    
    return "\n".join(sections)

# Example usage
print(generate_cstn_section(
    year=2000, month=1, day=1, time=0,
    num_levels=3,
    theta_v_ground=300,
    pressure_ground=100000,
    heights=[0, 1000, 20000],
    zonal_winds=[15, 20, 20],
    meridian_winds=[0, 0, 0],
    humidities=[0.007, 0.01]
))
def dict_to_namelist(data):
    """
    Convert a dictionary to a Fortran namelist format string.

    :param data: Dictionary containing the namelist data
    :return: A string formatted as a Fortran namelist
    """
    namelist_str = ""
    for section, values in data.items():
        namelist_str += f"&{section}\n"
        for key, value in values.items():
            if isinstance(value, bool):
                value = '.TRUE.' if value else '.FALSE.'
            elif isinstance(value, list):
                value = ', '.join(str(v) for v in value)
            namelist_str += f"  {key} = {value}\n"
        namelist_str += "/\n"
    return namelist_str

# Example dictionary
namelist_dict = {
    'NAM_CONFIO': {'LCDF4': True, 'LLFIOUT': True, 'LLFIREAD': False},
    'NAM_CONFZ': {'NZ_VERB': 5, 'NB_PROCIO_R': 1, 'NB_PROCIO_W': 8}
}

print(dict_to_namelist(namelist_dict))

def namelist_to_dict(namelist):
    """
    Convert a Fortran namelist format string to a dictionary.

    :param namelist: String in Fortran namelist format
    :return: Dictionary with the parsed data
    """
    import re
    pattern = re.compile(r"&(\w+)\s(.*?)\s/", re.S)
    items = pattern.findall(namelist)
    result = {}
    for section, content in items:
        entries = {}
        for line in content.strip().split('\n'):
            line = line.strip().split('!', 1)[0].strip()  # Remove comments
            if '=' in line:
                key, value = line.split('=', 1)
                key = key.strip()
                value = value.strip().replace("'", "")
                if value.lower() in ['.true.', '.false.']:
                    value = True if value.lower() == '.true.' else False
                elif ',' in value or '*' in value:  # List or repeated value
                    value = value.replace('*', ' ').split(',')
                    value = [v.strip() for v in value]
                entries[key] = value
        result[section] = entries
    return result

# Example namelist text
namelist_text = """
&NAM_CONFIO  LCDF4=T, LLFIOUT=T, LLFIREAD=F /
&NAM_CONFZ
   NZ_VERB=5 , NB_PROCIO_R=1 , NB_PROCIO_W=8
/
"""

print(namelist_to_dict(namelist_text))
def parse_gsd_sounding(file_path):
    """
    Parse a GSD sounding data file.

    :param file_path: Path to the GSD ASCII file
    :return: A dictionary with parsed sounding data
    """
    sounding_data = {}
    with open(file_path, 'r') as file:
        lines = file.readlines()
    
    data_lines = []

    for line in lines:
        # Clean up the line, removing leading/trailing whitespaces and handling empty lines
        clean_line = line.strip()
        if not clean_line:  # Skip empty lines
            continue

        parts = clean_line.split()
        try:
            line_type = int(parts[0])
        except ValueError:
            print(f"Skipping line due to formatting issues: {line}")
            continue
        
        if line_type in [1, 2, 3]:
            # Header lines processing
            if line_type == 1:
                sounding_data['Station Identification'] = {
                    'WBAN': parts[1],
                    'WMO': parts[2],
                    'LAT': parts[3],
                    'LON': parts[4],
                    'ELEV': parts[5],
                    'RTIME': parts[6]
                }
            elif line_type == 2:
                sounding_data['Sounding Checks'] = {
                    'HYDRO': parts[1],
                    'MXWD': parts[2],
                    'TROPL': parts[3],
                    'LINES': parts[4],
                    'TINDEX': parts[5],
                    'SOURCE': parts[6]
                }
            elif line_type == 3:
                sounding_data['Station Identifier and Other Indicators'] = {
                    'STAID': parts[1],
                    'SONDE': parts[4],
                    'WSUNITS': parts[5]
                }
        elif line_type >= 4:
            # Data lines processing
            data_lines.append({
                'PRESSURE': parts[1],
                'HEIGHT': parts[2],
                'TEMP': parts[3],
                'DEWPT': parts[4],
                'WIND DIR': parts[5],
                'WIND SPD': parts[6],
                'HHMM': parts[7],
                'BEARING': parts[8],
                'RANGE': parts[9]
            })
    
    sounding_data['Data Lines'] = data_lines
    return sounding_data



# Example usage
file_path = "/Users/filippi_j/Desktop/sounding.gsl"
sounding_section = parse_gsd_sounding(file_path)

import matplotlib.pyplot as plt
import metpy.plots as plots
from metpy.units import units
import numpy as np


import matplotlib.pyplot as plt
import numpy as np

def plot_sounding_data(sounding_data):
    """
    Plot basic line graphs for pressure, temperature, and dewpoint from the sounding data.
    
    :param sounding_data: Dictionary containing the parsed sounding data
    """
    # Extract the data
    pressures = []
    heights = []
    temperatures = []
    dewpoints = []
    wind_speeds = []
    
    for data_line in sounding_data['Data Lines']:
        # Check for and skip missing data indicated by '99999'
        if any(data_line[x] == '99999' for x in ['PRESSURE', 'HEIGHT', 'TEMP', 'DEWPT', 'WIND SPD']):
            continue
        
        pressures.append(float(data_line['PRESSURE']))
        heights.append(float(data_line['HEIGHT']))
        temperatures.append(float(data_line['TEMP']) / 10.0)  # Convert from tenths of degrees C to degrees C
        dewpoints.append(float(data_line['DEWPT']) / 10.0)  # Convert from tenths of degrees C to degrees C
        wind_speeds.append(float(data_line['WIND SPD']))
    
    # Convert lists to numpy arrays for plotting
    pressures = np.array(pressures)
    heights = np.array(heights)
    temperatures = np.array(temperatures)
    dewpoints = np.array(dewpoints)
    wind_speeds = np.array(wind_speeds)

    # Create plots
    fig, axs = plt.subplots(4, 1, figsize=(10, 20), sharex=True)

    # Plot each parameter
    axs[0].plot(heights, pressures, label='Pressure (hPa)', color='blue')
    axs[0].set_ylabel('Pressure (hPa)')
    axs[0].invert_yaxis()  # Invert axis to show decreasing pressure with height
    axs[0].legend(loc='best')

    axs[1].plot(heights, temperatures, label='Temperature (°C)', color='red')
    axs[1].set_ylabel('Temperature (°C)')
    axs[1].legend(loc='best')

    axs[2].plot(heights, dewpoints, label='Dew Point (°C)', color='green')
    axs[2].set_ylabel('Dew Point (°C)')
    axs[2].legend(loc='best')

    axs[3].plot(heights, wind_speeds, label='Wind Speed (knots)', color='purple')
    axs[3].set_ylabel('Wind Speed (knots)')
    axs[3].set_xlabel('Height (m)')
    axs[3].legend(loc='best')

    plt.tight_layout()
    plt.show()


def plot_tephigram(sounding_data):
    """
    Plot a tephigram from parsed GSD sounding data, ensuring correct plot of temperature and dew point.
    """
    # Extract pressure, temperature, and dewpoint data
    pressures = []
    temperatures = []
    dewpoints = []
    
    for data_line in sounding_data['Data Lines']:
        # Skip missing data represented by '99999'
        if data_line['PRESSURE'] == '99999' or data_line['TEMP'] == '99999' or data_line['DEWPT'] == '99999':
            continue

        pressure = float(data_line['PRESSURE'])
        temp = float(data_line['TEMP']) / 10.0  # Convert tenths of degrees C to degrees C
        dewpt = float(data_line['DEWPT']) / 10.0

        pressures.append(pressure)
        temperatures.append(temp)
        dewpoints.append(dewpt)

    pressures = np.array(pressures) * units.hPa
    temperatures = np.array(temperatures) * units.degC
    dewpoints = np.array(dewpoints) * units.degC

    fig = plt.figure(figsize=(10, 10))
    skew = plots.SkewT(fig, rotation=45)

    skew.plot_dry_adiabats(t0=np.arange(-30, 30, 10) * units.degC, alpha=0.25)
    skew.plot_moist_adiabats()
    skew.plot_mixing_lines()

    skew.plot(pressures, temperatures, 'red', label='Temperature')
    skew.plot(pressures, dewpoints, 'green', label='Dew Point')

    skew.ax.set_ylim(1000, 100)  # Set pressure limits
    skew.ax.set_xlim(-40, 50)  # Adjust temperature limits to ensure data fits within plot

    plt.legend(loc='upper right')
    plt.show()

# Example usage
plot_tephigram(sounding_section)

