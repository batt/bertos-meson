#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import glob
import hashlib
from shutil import copy2

def copyfiles(files, dest, sha):
    for f in files:
        #print(f)
        copy2(f, dest)
        _f = open(f, "r")
        sha.update(_f.read().encode())
        _f.close

app_dir = sys.argv[1]
outname = sys.argv[2]
build_dir = os.path.dirname(outname)

hw_glob = os.path.join(app_dir, "hw", "*.h")
cfg_glob = os.path.join(app_dir, "cfg", "*.h")

hw_files = glob.glob(hw_glob)
cfg_files = glob.glob(cfg_glob)

sha256 = hashlib.sha256()

hw_dir = os.path.join(build_dir, "hw")
cfg_dir = os.path.join(build_dir, "cfg")

try:
    os.mkdir(hw_dir)
except FileExistsError:
    pass

try:
    os.mkdir(cfg_dir)
except FileExistsError:
    pass

copyfiles(hw_files, hw_dir, sha256)
copyfiles(cfg_files, cfg_dir, sha256)
new_sha = "/* cfg+hw SHA256=%s */\n" % sha256.hexdigest()

try:
    outfile = open(outname, "r+")
except FileNotFoundError:
    outfile = open(outname, "w+")

old_sha = outfile.read()
print(f"Checking hw+cfg from {app_dir}...")
if old_sha != new_sha:    
    print("Copying files, saving new SHA256:", sha256.hexdigest())
    outfile.truncate(0)
    outfile.seek(0, os.SEEK_SET)
    outfile.write(new_sha)
else:
    print(f"No need to copy hw+cfg from {app_dir}")

outfile.close()




