# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = u'NDN Essential Tools'
copyright = u'Copyright © 2014-2021 Named Data Networking Project.'
author = u'Named Data Networking Project'

# The short X.Y version.
#version = ''

# The full version, including alpha/beta/rc tags.
#release = ''

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
#today = ''
# Else, today_fmt is used as the format for a strftime call.
today_fmt = '%Y-%m-%d'


# -- General configuration ---------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
needs_sphinx = '1.3'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
]

# The master toctree document.
master_doc = 'index'

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['Thumbs.db', '.DS_Store']


# -- Options for manual page output ------------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    ('ndnpeek',       'ndnpeek',       'simple consumer to send one Interest and receive one Data', [], 1),
    ('ndnpoke',       'ndnpoke',       'simple producer to publish one Data',        [], 1),
    ('ndnping',       'ndnping',       'reachability testing client',                [], 1),
    ('ndnpingserver', 'ndnpingserver', 'reachability testing server',                [], 1),
    ('ndnputchunks',  'ndnputchunks',  'producer program with content segmentation', [], 1),
    ('ndndump',       'ndndump',       'traffic analysis tool',                      [], 8),
    ('ndn-dissect',   'ndn-dissect',   'NDN packet format inspector',                [], 1),
]

# If true, show URL addresses after external links.
#man_show_urls = True
