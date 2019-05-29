#!c:\python27\python.exe
'''xxx utility.'''

VERSION = '1.0'

VER_FILE_TEMPL = '''/*
 * Automaticaly generated file.
 */
#ifndef _APP_VERSION_H_
#define _APP_VERSION_H_

#define APP_VERSION_STR     "%s"
#define APP_VERSION_MAJOR   %d
#define APP_VERSION_MINOR   %d

#endif // _APP_VERSION_H_
'''

def gen_version_info(version, filename):
    import os
    major = 0
    minor = 0
    fdot = version.find('.')
    if fdot >= 0:
        sdot = version.find('.', fdot+1)
        major = int(version[:fdot])
        minor = int(version[fdot+1:sdot])
    try:
        os.makedirs(os.path.dirname(filename))
    except OSError:
        pass
    with open(filename, 'w') as f:
        f.write(VER_FILE_TEMPL % (version, major, minor))


def get_target_toolchain():
    target = None
    toolchain = None
    try:
        with open(".mbed", 'r') as f:
            for line in f:
                line = line.rstrip()
                sep_pos = line.find('=')
                key = line[0:sep_pos]
                val = line[sep_pos+1:]
                if key == "TARGET":
                    target = val
                elif key == "TOOLCHAIN":
                    toolchain = val
    except IOError:
        pass
    return target, toolchain

if __name__ == '__main__':
    import getopt
    import os
    import sys
    import subprocess
    import re
    from shutil import copyfile

    usage = '''Release image building utility.
Usage:
    python build_release.py [options] VERSION

Arguments:
    VERSION     version string in the format major.minor.rel
                required only when no -d option is used

Options:
    -h, --help              this help message.
    -v, --version           version info.
    -n, --name=<name>       set image base name.
    -m, --target=<target>   set target name
    -t, --toolchain=<tc>    set toolchain name
    -d, --debug             build debug/engineer version.
'''

    proj_name = os.path.basename(os.getcwd())
    name = None
    start = None
    end = None
    size = None
    debug = False
    target, toolchain = get_target_toolchain()
    version_file = "version/app_version.h"

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hvn:m:t:d",
                                  ["help", "version", "name=", "target=", "toolchain=", "debug"])

        for o, a in opts:
            if o in ("-h", "--help"):
                print(usage)
                sys.exit(0)
            elif o in ("-v", "--version"):
                print(VERSION)
                sys.exit(0)
            elif o in ("-n", "--name"):
                try:
                    name = "%s" % a
                except:
                    raise getopt.GetoptError('Bad name value')
            elif o in ("-m", "--target"):
                target = a
            elif o in ("-t", "--toolchain="):
                toolchain = a
            elif o in ("-d", "--debug"):
                debug = True

        if name == None:
            # infer name from the project directory name
            name = proj_name

        if not debug:
            if not args:
                raise getopt.GetoptError('Release version is not specified')

            if len(args) > 2:
                raise getopt.GetoptError('Too many arguments')

            ver = args[0]
            match = re.search('\A\d+\.\d+\..*', ver)
            if match:
                ver = match.group(0)
            else:
                raise getopt.GetoptError('Incorrect version string format')
        else:
            ver = "eng_build"

        if not target:
            raise getopt.GetoptError('Target is not specified')
        
        if not toolchain:
            raise getopt.GetoptError('Toolchain is not specified')

    except getopt.GetoptError:
        msg = sys.exc_info()[1]     # current exception
        txt = 'ERROR: '+str(msg)  # that's required to get not-so-dumb result from 2to3 tool
        print(txt)
        print(usage)
        sys.exit(2)

    # check for version tag to not exist within git tree
    if not debug:
        dev_null = open(os.devnull, "w")
        res = subprocess.call(["git", "rev-parse", 'v'+ver], stdout=dev_null, stderr=subprocess.STDOUT)
        if res == 0:
            print('ERROR: Specified version already present in git.')
            sys.exit(3)

    # generate version information
    gen_version_info(ver, version_file)
    compile_cmd = ["mbed", "compile", "-m", target, "-t", toolchain]
    if debug:
        compile_cmd.append("--profile=mbed-os/tools/profiles/debug.json")
    else:
        compile_cmd.append("-c")

    res = subprocess.call(compile_cmd)
    if res != 0:
        print "Build has failed!"
        sys.exit(4)

    if not debug:
        release_filename = "releases/" + name + '-' + ver + ".hex"
        build_filename = "BUILD/"  + target + "/" + toolchain + "/" + proj_name + ".hex"

        copyfile(build_filename, release_filename)

        res = subprocess.call(["git", "add", release_filename ])
        res += subprocess.call(["git", "add", version_file ])
        if res != 0:
            print ("git error!")
            sys.exit(5)

        res = subprocess.call(["git", "commit", "-m" , "Release version " + ver])
        if res != 0:
            print ("git error")
            sys.exit(6)

        res = subprocess.call(["git", "tag", 'v'+ver ])
        if res != 0:
            print ("git error")
            sys.exit(7)

        print("\nRelease generated, commited and tagged. Please push it to remote repository.\n")
    else:
        print("Engineering build has been generated in BUILD folder.\n")
