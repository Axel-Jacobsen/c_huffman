#! /usr/bin/env python3

import time
import subprocess


if __name__ != "__main__":
    exit(1)

with open("enwik8", "rb") as asc:
    asc_str = asc.read()

for fsize in range(1024, len(asc_str), 1024):
    with open("testf", "wb") as f:
        f.write(asc_str[:fsize])

    commands = ["./huff", "testf"]
    try:
        s = subprocess.check_output(commands)
    except subprocess.CalledProcessError:
        print(f"failed on size {fsize // 1024} kb")
        break
    else:
        print(f"passed on size {fsize // 1024} kb")
