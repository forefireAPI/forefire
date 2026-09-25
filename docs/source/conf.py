# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import pathlib
import re

project = 'ForeFire'
copyright = '2014 - 2025, J-B Filippi'
author = 'Filippi, Jean Baptiste'

# Single source of truth for the version, the same file CMake and
# scikit-build-core read. Hard-coding it here left the site advertising 2.0.0
# for three releases.
_version_header = pathlib.Path(__file__).resolve().parents[2] / 'src' / 'include' / 'Version.h'
_match = re.search(r'ff_version\s*=\s*"v?([0-9]+\.[0-9]+\.[0-9]+)"',
                   _version_header.read_text(encoding='utf-8'))
if not _match:
	raise RuntimeError(f'could not parse ff_version from {_version_header}')
release = _match.group(1)
version = release

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
	'breathe'
]

breathe_projects = {
	"ForeFire": "../doxygen/xml"
}
breathe_default_project = "ForeFire"

templates_path = ['_templates']
exclude_patterns = []



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_logo = "_static/forefire.svg"
html_favicon = "_static/favicon.ico"

html_theme_options = {
	'logo_only': True,
	'style_nav_header_background': '#808080',
}

html_css_files = [
	"custom.css",
]
