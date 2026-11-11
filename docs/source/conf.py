# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import pathlib
import re

project = 'ForeFire'
copyright = '2014 - 2026, J-B Filippi'
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
	'breathe',
	'sphinx_design',
]

breathe_projects = {
	"ForeFire": "../doxygen/xml"
}
breathe_default_project = "ForeFire"

templates_path = ['_templates']
exclude_patterns = []



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'
html_static_path = ['_static']
html_favicon = "_static/favicon.ico"

html_title = "ForeFire"

html_theme_options = {
	'sidebar_hide_name': True,
	'light_logo': 'forefire.svg',
	'dark_logo': 'forefire-dark.svg',
	'light_css_variables': {
		'color-brand-primary': '#c2410c',
		'color-brand-content': '#c2410c',
		'color-brand-visited': '#9a3412',
	},
	'dark_css_variables': {
		'color-brand-primary': '#ff8a4c',
		'color-brand-content': '#ff8a4c',
		'color-brand-visited': '#f0a070',
	},
	'source_repository': 'https://github.com/forefireAPI/forefire/',
	'source_branch': 'dev',
	'source_directory': 'docs/source/',
}

