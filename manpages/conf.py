# General information about the project.
project = u'NDN Essential Tools'

master_doc = 'index'

# -- Options for manual page output ---------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    ('ndnpeek', 'ndnpeek', 'simple consumer to send one Interest and expect one Data', None, 1),
    ('ndnpoke', 'ndnpoke', 'simple producer to publish one Data', None, 1),
    ('ndnping', 'ndnping', 'reachability testing client', None, 1),
    ('ndnpingserver', 'ndnpingserver', 'reachability testing server', None, 1),
    ('ndndump', 'ndndump', 'traffic analysis tool', None, 8),
    ('ndn-dissect', 'ndn-dissect', 'NDN packet format inspector', None, 1),
    ('ndn-pib', 'ndn-pib',  'NDN PIB service', None, 1),
]
