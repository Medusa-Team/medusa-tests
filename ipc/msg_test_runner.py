#!/usr/bin/env python3

import subprocess
import os

subprocess_count = 300

def main():
    cwd = os.path.dirname(os.path.realpath(__file__))

    for i in range(subprocess_count):
        try:
            proc = subprocess.Popen('./msg_test', cwd=cwd)
        except:
            pass
        #print(proc.pid)

if __name__ == '__main__':
    main()
