#!/usr/bin/python3
import os
import sys
# import yaml
# import tarfile
# import tempfile
#import argparse
import subprocess
import multiprocessing
import shutil

localCachePath = os.environ.pop('KDECI_CACHE_PATH')
kritaCacheDir = os.path.join(localCachePath, 'krita-deps')
if not os.path.isdir(kritaCacheDir):
    os.makedirs(kritaCacheDir)

buildPath = os.path.join(os.getcwd(), '_build_plugins')
if os.path.isdir(buildPath):
    shutil.rmtree(buildPath)
os.makedirs(buildPath)

cpuCount = int(multiprocessing.cpu_count())
useCcacheForBuilds = os.environ.pop('KDECI_INTERNAL_USE_CCACHE') == 'True'

cmakeCommand = [
    # Run CMake itself
    'cmake',
    '-G Ninja',
    '-DINSTALL_ROOT={}'.format(os.path.join(os.getcwd(), '_install')),
    '-DEXTERNALS_DOWNLOAD_DIR={}'.format(kritaCacheDir),
    '-DCMAKE_BUILD_TYPE={}'.format(os.environ.pop('KDECI_BUILD_TYPE')),
    os.path.join(os.getcwd(), '3rdparty_plugins')
]

if useCcacheForBuilds:
    # Then instruct CMake accordingly...
    cmakeCommand.append('-DCMAKE_C_COMPILER_LAUNCHER=ccache')
    cmakeCommand.append('-DCMAKE_CXX_COMPILER_LAUNCHER=ccache')

    # Since we build external projects, we should propagate the 
    # ccache options via the environment variables...
    os.environ['CMAKE_C_COMPILER_LAUNCHER'] = 'ccache'
    os.environ['CMAKE_CXX_COMPILER_LAUNCHER'] = 'ccache'


commandToRun = ' '.join(cmakeCommand)

# Run the CMake command
try:
    print( "## RUNNING: " + commandToRun )
    subprocess.check_call( commandToRun, stdout=sys.stdout, stderr=sys.stderr, shell=True, cwd=buildPath)
except Exception:
    print("## Failed to configure plugins")
    sys.exit(1)

commandToRun = 'cmake --build . --target all --parallel {cpu_count}'.format(cpu_count = cpuCount)

# Run the CMake command
try:
    print( "## RUNNING: " + commandToRun )
    subprocess.check_call( commandToRun, stdout=sys.stdout, stderr=sys.stderr, shell=True, cwd=buildPath)
except Exception:
    print("## Failed to build plugins")
    sys.exit(1)
