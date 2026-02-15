#!/usr/bin/python

from __future__ import print_function
from subprocess import Popen, PIPE
from distutils.dir_util import copy_tree
import argparse
import os
import platform
import sys

win = platform.system() == 'Windows'
osx = platform.system() == 'Darwin'
linux = platform.system() == 'Linux'

plat = "undefined"
if win:
  plat = "win64"
elif osx:
  plat = "osx"
elif linux:
  plat = "linux64"

def create_output_dirs(out_dirs):
  for dir in out_dirs:
    out_path = os.path.join(os.getcwd(), dir)

    if not os.path.exists(out_path):
      os.makedirs(out_path)

def run_doxygen(doxy_files):
  for file in doxy_files:
    baseCmd = [
      'doxygen',
      file
    ]

    print("> Running doxygen. cwd={} cmd={}".format(os.getcwd(), baseCmd))
    rawResults = run(baseCmd)
    if rawResults[0] != 0:
      print (rawResults[2])
      exit(rawResults[0])

def main():
  if plat == "undefined":
    print("FATAL: Failed to detect platform. Exiting")
    exit(1)

  parser = argparse.ArgumentParser(description='Script to build mlsdk docs with Doxygen.')
  args = parser.parse_args()

  doxygenVersionResult = run_no_output([
    'doxygen',
    '--version'
  ])

  if doxygenVersionResult[0] is not 0:
    print ("! Error: doxygen not available at command line. sudo apt-get install doxygen")
    exit(1)

  out_dirs = [
    'docs/AppFramework'
  ]

  create_output_dirs(out_dirs)

  doxy_files = [
    'app_framework.dox'
  ]

  run_doxygen(doxy_files)

  exit(0)

def run(cmd, cwd='.'):
  p = Popen(' '.join(cmd), shell=True, stderr=sys.stderr, stdout=sys.stdout, universal_newlines=True, cwd=cwd)
  stdout, stderr = p.communicate()
  return (p.returncode, stdout, stderr)

def run_no_output(cmd):
  p = Popen(' '.join(cmd), shell=True, stderr=PIPE, stdout=PIPE, universal_newlines=True)
  stdout, stderr = p.communicate()
  return (p.returncode, stdout, stderr)

if __name__ == "__main__":
  main()
