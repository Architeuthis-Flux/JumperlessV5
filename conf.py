# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information
# import myst_parser
# import numpydoc
project = 'Jumperless V5'
copyright = '2024, Kevin Santo Cappuccio'
author = 'Kevin Santo Cappuccio'
release = '5.0.0.2'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'myst_parser',
    'sphinx.ext.autodoc',
    ]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_favicon = 'favicon.ico'
html_title = 'Jumperless V5'
html_logo = 'ColorJumpLogo200.png'
bodyfont = 'Helvetica, sans-serif'
headfont = 'Helvetica, sans-serif' 

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'
html_static_path = ['_static']
