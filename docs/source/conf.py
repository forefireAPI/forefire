# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'ForeFire'
copyright = '2014 - 2025, J-B Filippi'
author = 'Filippi, Jean Baptiste'
release = '2.0.0'

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
