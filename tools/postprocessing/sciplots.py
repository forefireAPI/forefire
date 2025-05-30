#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Oct 24 11:18:16 2023

@author: filippi_j
"""
import matplotlib.pyplot as plt
from scipy.fftpack import fftshift, fft2
from scipy import stats
import numpy as np
import xarray as xr

def generate_3D_field(nx, ny, nz, dx,nf = 8):
    """
    Génère un champ 3D en superposant des ondes de différentes longueurs d'onde pour tests de fonctions

    Paramètres :
    - nx, ny, nz : dimensions du champ 3D
    - dx : intervalle entre les points du champ

    Retourne :
    - field_3D : array 3D numpy représentant le champ
    """
    # Coordonnées 2D
    x = np.linspace(0, nx*dx, nx)
    y = np.linspace(0, ny*dx, ny)
    
    # Grilles de coordonnées 2D
    X, Y = np.meshgrid(x, y)
    
    # Initialisation du champ 2D à zéro
    field_2D = np.zeros_like(X)
    
    # Génération du champ 2D avec des ondes de différentes longueurs d'onde
    
    for wavelength in np.linspace(20*dx, 100*dx, nf):
        print(wavelength)
        k = 2 * np.pi / wavelength
        R = np.sqrt(X**2 + Y**2)
        field_2D += np.sin(k * R )
    
    
    # Création du champ 3D en copiant le champ 2D sur tous les niveaux
    field_3D = np.repeat(field_2D[np.newaxis, :, :], nz, axis=0)

    return field_3D

    
def compute_radial_spectrum(image):
    """
    Compute the radial energy spectrum of a 2D field.

    Parameters:
    - image : 2D numpy array representing the field

    Returns:
    - radial_spectrum : 1D numpy array containing the radial energy spectrum
    """
    # Compute 2D Fourier Transform
    f_transform = fft2(image)
    f_transform = fftshift(f_transform)

    # Compute power spectrum
    power_spectrum = np.abs(f_transform) ** 2

    # Initialize variables
    center = np.array([power_spectrum.shape[0] // 2, power_spectrum.shape[1] // 2])
    max_radius = int(np.linalg.norm(center))
    radial_spectrum = np.zeros(max_radius)

    # Loop through radii to compute radial spectrum
    for r in range(max_radius):
        y, x = np.ogrid[-center[0]:center[0], -center[1]:center[1]]
        mask = (x * x + y * y >= r * r) & (x * x + y * y < (r + 1) * (r + 1))
        radial_spectrum[r] = np.sum(power_spectrum[mask])

    return radial_spectrum



def compute_3D_radial_spectrum(field_3D):
    """
    Compute the 3D radial energy spectrum of a 3D field.

    Parameters:
    - field_3D : 3D numpy array representing the field

    Returns:
    - radial_spectrum_3D : 1D numpy array containing the 3D radial energy spectrum
    """
    # Compute 3D Fourier Transform
    f_transform_3D = fftshift(fft2(field_3D))
    
    # Compute power spectrum
    power_spectrum_3D = np.abs(f_transform_3D) ** 2

    # Initialize variables
    nz, ny, nx = power_spectrum_3D.shape
    center = np.array([ny // 2, nx // 2])  # Note: The 3D FFT is a stack of 2D FFTs
    max_radius = int(np.sqrt(center[0]**2 + center[1]**2))
    radial_spectrum_3D = np.zeros(max_radius)

    # Loop through all 2D slices to sum their radial spectra
    for z in range(nz):
        for r in range(max_radius):
            y, x = np.ogrid[-center[0]:center[0], -center[1]:center[1]]
            mask = (x * x + y * y >= r * r) & (x * x + y * y < (r + 1) * (r + 1))
            radial_spectrum_3D[r] += np.sum(power_spectrum_3D[z, mask])

    return radial_spectrum_3D



def plot_radial_spectrum(field, dx, show_marker=True, show_slope=False, slope_offset=1.0, var_name=""):
    """
    Plot the radial energy spectrum of a 2D or 3D field in log-log scale.

    Parameters:
    - field : 2D or 3D numpy array representing the field
    - dx : interval between the points of the field
    - show_marker : boolean indicating whether to show the 2*dx marker
    - show_slope : boolean indicating whether to show the -5/3 slope line
    - slope_offset : float indicating the vertical offset for the -5/3 slope line

    Returns:
    - None
    """
    # Determine if the field is 2D or 3D and compute the appropriate radial spectrum
    if len(field.shape) == 2:
        radial_spectrum = compute_radial_spectrum(field)
    elif len(field.shape) == 3:
        radial_spectrum = compute_3D_radial_spectrum(field)
    else:
        raise ValueError("Field must be either 2D or 3D.")

    # Convert radial frequency to wavelength
    nyquist_wavelength = 2 * dx
    num_points = len(radial_spectrum)
    wavelengths = nyquist_wavelength * np.linspace(1, num_points, num_points) / num_points

    # Plot the radial energy spectrum in log-log scale
    plt.figure(figsize=(8, 6))
    plt.loglog(((np.pi))/wavelengths, radial_spectrum, label=f'{var_name} energy')
    
    if show_marker:
        plt.axvline(x=10*dx, color='r', linestyle='--', label=f'2*dx = {2*dx}')
    
    if show_slope:
        # Generate a line with a -5/3 slope for comparison
        k_min = np.min(wavelengths)
        k_max = np.max(wavelengths)
        c = radial_spectrum[int(num_points / 4)] * slope_offset  # A reference point for normalization
        slope_line = c * (wavelengths / k_min) ** (-5 / 3)
        plt.loglog(wavelengths, slope_line, 'k--', label='-5/3 slope')
    
    plt.title(f'Energy Spectrum {var_name}')
    plt.xlabel('Wavelength (m)')
    plt.ylabel('Energy m^2.s^-2')
    plt.grid(True)
    
    plt.legend()
    
    plt.show()
def radial_profile(data):
    y, x = np.indices((data.shape))
    center = np.array([(x.max() - x.min()) / 2.0, (y.max() - y.min()) / 2.0])
    r = np.sqrt((x - center[0]) ** 2 + (y - center[1]) ** 2)
    r = r.astype(np.int)
    radial_mean = np.zeros(r.max() + 1)
    for i in range(r.max() + 1):
        radial_mean[i] = np.mean(data[r == i])
    return radial_mean
def plot_radial_spectrum_inverted_correctly(field_2D):
    F = np.fft.fftshift(np.fft.fft2(field_2D))
    psd2D = np.abs(F) ** 2
    radial_mean = radial_profile(psd2D)
    k = np.linspace(1, len(radial_mean) - 1, len(radial_mean) - 1)
    radial_mean = radial_mean[1:]
    wavelength = 2 * np.pi / k

    plt.loglog(wavelength, radial_mean, label='Spectre radial')
    plt.loglog(wavelength, radial_mean[0] * (k[0]/k) ** (5 / 3), '--', label=r'$k^{-5/3}$')
    
    plt.xlabel('Longueur d\'onde')
    plt.ylabel('Spectre de puissance')
    plt.legend()
    
    # Inversion de l'axe des abscisses
    plt.gca().invert_xaxis()
    plt.show()

# Utilisation de la même field_2D pour cet exemple


my_file ="/Users/filippi_j/Volumes/fcouto/KTEST_PEDROGAO/001_2FIRES/006_Runff/JUN17.3.EXP03.041.nc"
my_var = "WT"
ds = xr.open_dataset(my_file)
D = ds[my_var][0].values
dx = float(ds.XHAT[1]-ds.XHAT[0])
# plot_radial_spectrum(D[2], dx, slope_offset=1000.00, var_name= ds[my_var].standard_name)
dx = 1
field_3D = D#generate_3D_field(200, 200, 10, dx, nf=2)

# Extract the 3rd level (index 2) for plotting
third_level = field_3D[2, :, :]
plot_radial_spectrum_inverted_correctly(third_level)

# Plot the 3rd level
plt.figure(figsize=(8, 8))
plt.imshow(third_level, cmap='viridis', extent=[0, 200*3, 0, 200*3])
plt.colorbar(label='Field Value')
plt.title('3rd Level of the 3D Field')
plt.xlabel('x-coordinate')
plt.ylabel('y-coordinate')
plt.show()
#plot_radial_spectrum(field_3D[2], dx)
