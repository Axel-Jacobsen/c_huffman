#! /usr/bin/env python3

import time
import subprocess


if __name__ != "__main__":
    exit(1)


with open("all_ascii", "rb") as asc:
    asc_str = asc.read()

for fsize in range(1, 24):
    with open("testf", "wb") as f:
        f.write(asc_str * (1 << fsize))

    full_size = len(asc_str) * (1 << fsize)

    commands = ["./huff", "testf"]
    try:
        s = subprocess.check_output(commands)
    except subprocess.CalledProcessError:
        print(f"failed on size {full_size} bytes")
        print(s)
    else:
        print(f"passed on size {full_size} bytes")
