import os
from setuptools import setup

# Utility function to read the README file.
# Used for the long_description.  It's nice, because now 1) we have a top level
# README file and 2) it's easier to type in the README file than to put a raw
# string in below ...
# From: https://pythonhosted.org/an_example_pypi_project/setuptools.html
def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

exec(open('upvtc_ct/_version.py').read())
setup(
    name='upvtc_ct',
    version=__version__,
    author='Sean Francis N. Ballais',
    author_email='snballais@up.edu.ph',
    description='An automated timetabler for UPVTC.',
    long_description=read('README.md'),
    license='GPL',
    keywords='scheduling thesis',
    url='https://github.com/seanballais/upvtc-course-timetabler',
    install_requires=[
        'pyqt5',
        'peewee>=3.0.0',
        'docopt'
    ],
    packages=[ 'upvtc_ct' ],
    entry_points={
        'console_scripts': [ 'upvtc_ct=upvtc_ct.app:main' ]
    },
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Topic :: Utilities',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
    ],
)
