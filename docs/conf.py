# -*- coding: utf-8 -*-

# -- Metadata about this file ------------------------------------------------
__copyright__ = "Copyright (c) 2020 Arm Limited"
__license__ = "SPDX-License-Identifier: BSD-3-Clause"

# Configuration file for the Sphinx documentation builder.

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

# -- Project information -----------------------------------------------------
project = 'Trusted Services'

# The full version, including alpha/beta/rc tags
with open('../version.txt', 'r') as f:
  release = f.read()
  f.close()
version=release

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ['sphinx.ext.autosectionlabel', 'sphinxcontrib.plantuml',
              'sphinxcontrib.moderncmakedomain', 'sphinx.ext.todo']

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = None

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# Load the contents of the global substitutions file into the 'rst_prolog'
# variable. This ensures that the substitutions are all inserted into each page.
with open('global_substitutions.txt', 'r') as subs:
  rst_prolog = subs.read()
# Minimum version of sphinx required
needs_sphinx = '2.0'

# -- Options for HTML output -------------------------------------------------

# Don't show the "Built with Sphinx" footer
html_show_sphinx = False

# Don't show copyright info in the footer (we have this content in the page)
html_show_copyright = False

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = "sphinx_rtd_theme"

# The logo to display in the sidebar
html_logo = '_static/images/tf_logo_white.png'


# Options for the "sphinx-rtd-theme" theme
html_theme_options = {
  'collapse_navigation': False,  # Can expand and collapse sidebar entries
  'prev_next_buttons_location': 'both',  # Top and bottom of the page
  'style_external_links': True  # Display an icon next to external links
}

html_static_path = ['_static']

#Add custom css for HTML. Used to allow full page width rendering
def setup(app):
    app.add_stylesheet('css/custom.css')


# -- Options for autosectionlabel --------------------------------------------

# Only generate automatic section labels for document titles
autosectionlabel_maxdepth = 1

# -- Options for plantuml ----------------------------------------------------

plantuml_output_format = 'svg_img'

# -- Options for todo extension ----------------------------------------------

# Display todos
todo_include_todos = True