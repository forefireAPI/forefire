import matplotlib.pyplot as plt
from matplotlib.widgets import Button
import numpy as np

# Sample params dictionary
params_dict = {
    "BB": {"rhoA": 1, "rhoB": 1},
    "BC": {"rhoA": 2, "rhoB": 1},
    "BD": {"rhoA": 3, "rhoB": 1},
}


# Placeholder for the external function
def speed(DIR, VnormBlue, VangleBlue, VnormRed, VangleRed, params):
    # Replace this with the actual function logic
    # Example: using params['rhoA'] and params['rhoB']
    return (
        np.sin(np.deg2rad(DIR - VangleBlue)) * VnormBlue * params["rhoA"]
        + np.cos(np.deg2rad(DIR - VangleRed)) * VnormRed * params["rhoB"]
    )


# Function to calculate the norm and angle of a vector
def vector_properties(u, v):
    norm = np.sqrt(u**2 + v**2)
    angle = np.rad2deg(np.arctan2(v, u))
    return norm, angle


# Global variables to store vector states
move_vector = "None"
vnorm_red, vangle_red = 1, 1
vnorm_blue, vangle_blue = -1, 1

# This function will be called when the mouse is clicked
def on_click(event):
    global move_vector
    if event.button == 1:  # Left click
        if move_vector == "None":
            move_vector = "Red"
        elif move_vector == "Red":
            move_vector = "Blue"
        else:
            move_vector = "None"


# This function will be called when the mouse is moved
def on_move(event):
    global vnorm_red, vangle_red, vnorm_blue, vangle_blue

    if event.inaxes != ax_cartesian:
        return

    if move_vector == "Red":
        # Update red vector
        u, v = event.xdata - center[0], event.ydata - center[1]
        red_vector.set_UVC(u, v)
        vnorm_red, vangle_red = vector_properties(u, v)
        red_text.set_text(f"Red: Vnorm: {vnorm_red:.2f}, Vangle: {vangle_red:.2f}°")
    elif move_vector == "Blue":
        # Update blue vector
        u, v = event.xdata - center[0], event.ydata - center[1]
        blue_vector.set_UVC(u, v)
        vnorm_blue, vangle_blue = vector_properties(u, v)
        blue_text.set_text(f"Blue: Vnorm: {vnorm_blue:.2f}, Vangle: {vangle_blue:.2f}°")

    # Update the polar plot
    update_plot()


# Function to update the plot based on the current state
def update_plot():
    title_text = f"Current Params"
    ax_cartesian.set_title(title_text)
    ax_polar.clear()
    rangeDir = range(0, 360, 3)

    # Prepare data for the polar plot
    r_values = [
        speed(DIR, vnorm_blue, vangle_blue, vnorm_red, vangle_red, current_params)
        for DIR in rangeDir
    ]
    theta_values = np.deg2rad(rangeDir)

    # Clearing and redrawing the polar plot
    ax_polar.clear()
    ax_polar.plot(theta_values, r_values, "r-")  # Draw a line instead of points
    ax_polar.set_theta_zero_location("N")
    ax_polar.set_theta_direction(-1)
    ax_polar.set_ylim(0, 5)  # Adjust as needed
    fig.canvas.draw_idle()


def plot_polar(params_dict):
    # Global variable for the current parameters
    current_params = params_dict["BB"]

    # Create a figure with a specific size
    fig = plt.figure(figsize=(12, 8))

    # Adjust the subplots to make room for buttons
    plt.subplots_adjust(left=0.1, bottom=0.25, right=0.9, top=0.95)

    # Adding a Cartesian subplot
    ax_cartesian = fig.add_subplot(121)
    ax_cartesian.set_xlim(0, 10)
    ax_cartesian.set_ylim(0, 10)
    ax_cartesian.set_aspect("equal", "box")

    # Adding a Polar subplot
    ax_polar = fig.add_subplot(122, polar=True)
    ax_polar.set_theta_zero_location("N")  # North at the top
    ax_polar.set_theta_direction(-1)  # Clockwise

    # Center of the vectors
    center = (5, 5)
    # Initial values for the vectors
    initial_u_red, initial_v_red = 1, 1
    initial_u_blue, initial_v_blue = -1, 1

    # Calculate initial norms and angles
    vnorm_red, vangle_red = vector_properties(initial_u_red, initial_v_red)
    vnorm_blue, vangle_blue = vector_properties(initial_u_blue, initial_v_blue)

    # Creating the initial red vector and text
    red_vector = ax_cartesian.quiver(
        *center,
        initial_u_red,
        initial_v_red,
        angles="xy",
        scale_units="xy",
        scale=1,
        color="r",
    )
    red_text = ax_cartesian.text(
        0.05,
        0.95,
        f"Red: Vnorm: {vnorm_red:.2f}, Vangle: {vangle_red:.2f}°",
        transform=ax_cartesian.transAxes,
        color="r",
    )

    # Creating the initial blue vector and text
    blue_vector = ax_cartesian.quiver(
        *center,
        initial_u_blue,
        initial_v_blue,
        angles="xy",
        scale_units="xy",
        scale=1,
        color="b",
    )
    blue_text = ax_cartesian.text(
        0.05,
        0.90,
        f"Blue: Vnorm: {vnorm_blue:.2f}, Vangle: {vangle_blue:.2f}°",
        transform=ax_cartesian.transAxes,
        color="b",
    )

    # Connecting the events
    fig.canvas.mpl_connect("button_press_event", on_click)
    fig.canvas.mpl_connect("motion_notify_event", on_move)

    button_width = 0.1
    button_spacing = 0.005
    total_button_width = (
        len(params_dict) * (button_width + button_spacing) - button_spacing
    )
    start_x = (1 - total_button_width) / 2  # Center the button row

    buttons = []  # Keep a reference to the buttons

    for i, key in enumerate(params_dict.keys()):
        ax_button = plt.axes(
            [start_x + i * (button_width + button_spacing), 0.05, button_width, 0.1]
        )
        button = Button(ax_button, key)

        # Define and assign the handler function directly
        def button_handler(event, key=key):  # Default argument captures the current key
            global current_params
            current_params = params_dict[key]
            print(f"Button pressed: {key}")
            update_plot()

        button.on_clicked(button_handler)
        buttons.append(button)  # Keep a reference

    plt.show()
