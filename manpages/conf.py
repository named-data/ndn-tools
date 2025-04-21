# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'NDN Essential Tools'
copyright = 'Copyright © 2014-2025 Named Data Networking Project.'
author = 'Named Data Networking Project'

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
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

needs_sphinx = '4.0'
extensions = []

exclude_patterns = ['Thumbs.db', '.DS_Store']


# -- Options for manual page output ------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-manual-page-output

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    ('ndndissect',    'ndndissect',    'NDN packet format inspector',               [], 1),
    ('ndndump',       'ndndump',       'NDN traffic analysis tool',                 [], 8),
    ('ndnpeek',       'ndnpeek',       'simple NDN consumer to send one Interest and receive one Data', [], 1),
    ('ndnpoke',       'ndnpoke',       'simple NDN producer to publish one Data',   [], 1),
    ('ndnping',       'ndnping',       'NDN reachability testing client',           [], 1),
    ('ndnpingserver', 'ndnpingserver', 'NDN reachability testing server',           [], 1),
    ('ndnserve',      'ndnserve',      'NDN producer with content segmentation',    [], 1),
]
