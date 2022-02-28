#!/usr/bin/env python3

import os
import sys
from getpass import getuser

# workspace_path = "/home/" + getuser() + "/catkin_ws"
# build_path = "/home/" + getuser() + "/catkin_ws/build"
root_dir = f"{sys.path[0]}/.."

target_ccjson_path = root_dir +'/compile_commands.json'


if os.path.isfile(target_ccjson_path):
    os.system('rm %s' % target_ccjson_path)
    print("removed previous compile_commands.json located at %s" % target_ccjson_path)


file_paths = {}
file_paths['compile_commands.json'] = [
    f"{root_dir}/CA7/linux-5.10.10",
    f"{root_dir}/CM4",
    f"{root_dir}/CA7/stm32-ecu-manager"
]

# for each file in the dictionary, concatenate
# the content of the files in each directory
# and write the merged content into a file
# with the same name at the top directory
for f, paths in file_paths.items():
    txt = []
    p = os.path.join(paths[0], f)
    if os.path.islink(p):
        with open(os.path.join(os.path.dirname(p), os.readlink(p))) as f2:
            txt.append('['+f2.read()[1:-2]+',')
    else:
        with open(p) as f2:
            txt.append('['+f2.read()[1:-2]+',')
    
    for p in paths[1:-1]:
        pth = os.path.join(p, f)
        if os.path.islink(pth):
            with open(os.path.join(os.path.dirname(pth), os.readlink(pth))) as f2:
                txt.append(f2.read()[1:-2]+',')
        else:
            with open(pth) as f2:
                txt.append(f2.read()[1:-2]+',')
    
    p = os.path.join(paths[-1], f)
    if os.path.islink(p):
        with open(os.path.join(os.path.dirname(p), os.readlink(p))) as f2:
            txt.append(f2.read()[1:-1]+']')
    else:
        with open(p) as f2:
            txt.append(f2.read()[1:-1]+']')
    with open(f, 'w') as f3:
        f3.write(''.join(txt))

if not os.path.isfile(target_ccjson_path):
    os.system('pwd')
    os.system('mv ./compile_commands.json %s' % (target_ccjson_path))


print("\033[1;32mFinished writing compile_commands.json\033[0m")
