#!/usr/bin/python

import argparse
import collections
import os
import platform
import re
import shutil
import sys

sys.path.insert(0, '../scripts')
from mlsdk_helpers import *
from vs_helper import get_ninja_env

win = platform.system() == 'Windows'
osx = platform.system() == 'Darwin'
linux = platform.system() == 'Linux'

ext = ".so"
plat = "undefined"
if win:
  plat = "win"
  ext = ".dll"
elif osx:
  plat = "osx"
  ext = ".dylib"
elif linux:
  plat = "linux"
  ext = ".so"


def main():
  if plat == "undefined":
    print("FATAL: Failed to detect platform. Exiting")
    exit(1)

  parser = argparse.ArgumentParser(description='Script to assist in generating and building the app framework.')
  parser.add_argument('--install', dest='base_install_path', default=base_install_path, help='Absolute path to directory to be used for installation. Note: If this directory already exists contents will be overwritten.')
  parser.add_argument('--jobs', '-j', dest='jobs', help='Number of build parallel build jobs')
  parser.add_argument('--version', dest='version_string', default=None, help='Version information to insert into the ml_version header. Format is "major.minor.revision.build_id"')
  parser.add_argument('--verbose', '-v', dest='verbose', default=False, action='store_true', help='Enable verbose output')
  parser.add_argument('--build', '-b', dest='build', nargs='*', choices=TARGETS_CHOICE, type=str.lower,
                      help='Builds app framework for selected targets. Assumes both targets, when called with no values.')
  parser.add_argument('--clean', '-c', dest='clean', nargs='*', choices=TARGETS_CHOICE, type=str.lower,
                      help='Deletes app framework\'s build folder for selected targets. Assumes both targets, when called with no values.')
  parser.add_argument('--config', dest='config', nargs='*', choices=CONFIG_CHOICE, type=str.lower, default=CONFIG_CHOICE,
                      help='Specifies build configuration. Assumes release target, when called with no values.')
  parser.add_argument('--legacy', dest='legacy', default=False, action='store_true', help='Build legacy configurations (pre v1.7.0)')


  if ( plat == "win"):
    parser.add_argument('--msvc', dest='msvc', default=None, help="Selects specific Visual Studio version to use.")
    parser.add_argument('--no-rescan', action='store_false', default=True, dest='force_vs_rescan', help="Forces no rescan of Visual Studio environments.")

  args = parser.parse_args()
  translate_default_arguments(args)

  print("\n\n> Args: \n\t", args)

  if (HOST in args.build or HOST in args.clean) and not args.legacy:
    print("\n\nFATAL: Host config is not supported starting 1.7.0 release.")
    exit(1)

  print("\n\n>Building App Framework")
  print("Arguments passed to the script : ", args)
  args.force_vs_rescan = getattr(args, 'force_vs_rescan', True)
  args.msvc = getattr(args, 'msvc', None)

  if os.environ.get("MLSDK") is None:
    print("FATAL: Failed to detect MLSDK environment variable. Exiting")
    exit(1)

  args.ANDROID_NDK_ROOT   = get_ndk_path('25.0.8775105')
  args.ANDROID_CMAKE_PATH = get_cmake_path()
  args.ANDROID_TOOLCHAIN  = os.path.join(args.ANDROID_NDK_ROOT, 'build', 'cmake', 'android.toolchain.cmake')
  args.root_path          = root_path

  if args.clean:
    if DEVICE in args.clean and HOST in args.clean:
      print("\n\n> Removing all the build directories.")
      shutil.rmtree(build_path, ignore_errors=True)
    elif HOST in args.clean:
      print("\n\n> Removing host build directory.")
      shutil.rmtree(host_build_path, ignore_errors=True)
    elif DEVICE in args.clean:
      print("\n\n> Removing device build directory.")
      shutil.rmtree(device_build_path, ignore_errors=True)

    # if no build argument provided finish the script
    if not args.build:
      return

  ver_string = None
  if args.version_string:
    version = ExtractVersion(args.version_string)
    ver_string = version.major+'.'+version.minor+'.'+version.patch
    # ToDo: CMake is not taking the 4th version (i.e. build_id). Commenting
    #       this section of the code till I figure it out.
    #
    #if version.build_id:
    #  ver_string += '-'+version.build_id

  if args.build:
    # Build and install the app framework for the host [Linux/Windoes/MacOS] platform.
    if HOST in args.build:
      ProduceBuild(args, plat, host_build_path, ver_string, args.verbose)

    # Build and install the app framework for the Lumin [ML2] platform.
    if DEVICE in args.build:
      ProduceBuild(args, "ml2", device_build_path, ver_string, args.verbose)

  print("\n\n>App Framework is successfully generated and installed here: ")
  print("   ", args.base_install_path)
  print("To use this App Framework point your MAGICLEAP_APP_FRAMEWORK_ROOT env variable to this installed location.")

def DetectClangArchitectures():
    if not osx:
        return []

    comp = subprocess.run(['clang', '--version'], stdout=subprocess.PIPE, universal_newlines=True)

    archs = ['x86_64']
    if comp.returncode != 0:
        # this would be really bad (until proven otherwise)
        print("could not run 'clang', assuming Intel-only for now")
        return archs

    # Apple clang version 12.0.0 (clang-1200.0.32.28) ...
    m = re.search(r'Apple clang version (\d+)', comp.stdout)
    if not m:
        print("could not detect version info from 'clang', assuming Intel-only for now\n%s" % comp.stdout)
        return archs

    if int(m.group(1)) >= 12:
        # Xcode/clang 12 is where M1 support added
        archs.append('arm64')

    return archs


def ProduceBuild(build_args, target, build_base_dir, ver_str=None, verbose=False):

  cmake_exec = os.path.join(build_args.ANDROID_CMAKE_PATH, 'cmake')

  for config in build_args.config:

    install_path = os.path.join(build_args.base_install_path, target, config.capitalize()).replace('\\', '/')
    build_path =  os.path.join(build_base_dir, config)
    os.makedirs(build_path, exist_ok=True)
    os.chdir(build_path)

    print("\n\n> Building app_framework for {} {}".format(target, config))

    cmd = [ cmake_exec ]
    if target == "ml2":
      cmd += [ '-DCMAKE_TOOLCHAIN_FILE=%s' % build_args.ANDROID_TOOLCHAIN,
               '-DANDROID_NDK=%s' % build_args.ANDROID_NDK_ROOT,
               '-DANDROID_ABI=x86_64',
               '-DANDROID_NATIVE_API_LEVEL=29',
               '-GNinja' ]

      # workaround for using non AndroidStudio-cmake-bundled ninja
      ninja_path = '{}/ninja'.format(build_args.ANDROID_CMAKE_PATH)
      if get_app_version(ninja_path) is None:
        # try to find a ninja path for current msvc
        custom_env = get_ninja_env(build_args.force_vs_rescan, build_args.msvc)
        env_path = custom_env.get('PATH') if custom_env else None
        found_ninja_path = get_ninja_path(env_path)
        if found_ninja_path:
          ninja_path = found_ninja_path
      cmd += ['-D', 'CMAKE_MAKE_PROGRAM={}'.format(ninja_path)]

    if osx:
      cmd += [ '-DCMAKE_OSX_ARCHITECTURES=' + ';'.join(DetectClangArchitectures()) ]

    cmd += [ '-DCMAKE_INSTALL_PREFIX=%s' % install_path,
             '-DCMAKE_BUILD_TYPE=%s' % config ]

    # use ninja if possible
    custom_env = get_ninja_env(build_args.force_vs_rescan, build_args.msvc)
    if custom_env:
      ninja_path = get_ninja_path(custom_env.get('PATH'))
      if ninja_path:
        cmd += ['-D', 'CMAKE_MAKE_PROGRAM={}'.format(ninja_path), '-G', 'Ninja']

    # if we didn't find ninja, revert to msvc
    if target == "win" and not ninja_path:
      msvc = build_args.msvc if build_args.msvc else "Visual Studio 16 2019"
      cmd += ['-G', msvc, '-A', 'x64']

    if ver_str:
      cmd += [ '-DAPP_FRAMEWORK_VERSION=' + ver_str ]

    cmd += [ '../../..' ]

    print("\nCreating platform specific make files for {} config".format(config))
    results = run(cmd, verbose)

    print("\nBuild app_framework for {} config, this can take several minutes.".format(config))
    cmd = [ cmake_exec, '--build', '.' ]
    if build_args.jobs is not None:
      cmd += [ '-j', str(build_args.jobs) ]

    cmd += [ '--config', config ]
    results = run(cmd, verbose)

    print("\nInstall {} {} app_framework to: {}".format(target,config,install_path))
    cmd = [ cmake_exec, '--install', '.' ]
    cmd += [ '--config', config ]
    results = run(cmd, verbose)

    print("\nCopying FindMagicLeapAppFramework.cmake.")
    os.makedirs(os.path.join(build_args.base_install_path, 'cmake'), exist_ok=True)

    # List of [source, dest] filenames.
    files = [ ["FindMagicLeapAppFramework.cmake.prebuild","FindMagicLeapAppFramework.cmake"] ]
    for file in files:
      shutil.copy(os.path.join(root_path, 'cmake', file[0]),
                os.path.join(build_args.base_install_path, 'cmake', file[1]) )

    # Inject the version provided in the command line to the ml_version.h.
    if build_args.version_string:
      version = ExtractVersion(build_args.version_string)

      with open(os.path.join(install_path, 'include', 'app_framework', 'version.h'), "r") as version_file:
        version_header_contents = version_file.read()

      version_header_contents = version_header_contents.replace("ML_APP_FRAMEWORK_VERSION_MAJOR 0", "ML_APP_FRAMEWORK_VERSION_MAJOR " + version.major)
      version_header_contents = version_header_contents.replace("ML_APP_FRAMEWORK_VERSION_MINOR 0", "ML_APP_FRAMEWORK_VERSION_MINOR " + version.minor)
      version_header_contents = version_header_contents.replace("ML_APP_FRAMEWORK_VERSION_REVISION 0", "ML_APP_FRAMEWORK_VERSION_REVISION " + version.patch)

      if version.build_id:
        version_header_contents = version_header_contents.replace("ML_APP_FRAMEWORK_VERSION_BUILD_ID \"0", "ML_APP_FRAMEWORK_VERSION_BUILD_ID \"" + version.build_id)

      with open(os.path.join(install_path, 'include', 'app_framework/version.h'), "w") as version_file:
       version_file.write(version_header_contents)

  os.chdir(root_path)
  return results

# Inject version info supplied on the command line into the ml_version header
# Params:
#   version: Version information to insert into the ml_version header.
#            Format is "major.minor.revision.build_id"
#
def ExtractVersion(version):
  version_pattern = "([0-9]+)\.([0-9]+)\.([0-9]+)\.?(.+)?"
  version_obj = re.match(version_pattern, version)
  if not version_obj:
    print("Invalid version specified [ " + version + " ]. Exiting.")
    exit(1)

  major = version_obj.group(1)
  minor = version_obj.group(2)
  patch = version_obj.group(3)
  build_id = None

  if version_obj.group(4):
    build_id = version_obj.group(4)

  version = Version(major, minor, patch, build_id)
  return version

if __name__ == "__main__":

  root_path           = os.path.dirname(os.path.realpath(__file__))
  build_path          = os.path.join(root_path, 'build')
  device_build_path   = os.path.join(build_path, 'build_device')
  host_build_path     = os.path.join(build_path, 'build_host')
  base_install_path   = os.path.join(build_path, 'install')

  Version = collections.namedtuple('Version', ['major', 'minor',
                                   'patch', 'build_id'])

  main()
