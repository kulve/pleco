#! /usr/bin/env python3

#
# Consume video frames
#
import sys
import time
import os

def main(argv):
    shared_mem = ''
    print("OD: ready")
    sys.stdout.flush()

    uin = os.fdopen(sys.stdin.fileno(), 'rb', buffering=0)

    while True:
        # Make sure the first byte is 'A'
        A = uin.read(1)
        if len(A) == 0:
            break

        if ord(A) != ord('A'):
            print("OD: Unexpected input (A):", ord(A))
            sys.stdout.flush()
            continue

        # width divided by 8 to fit to byte
        WD3 = uin.read(1)
        # height divided by 8 to fit to byte
        HD3 = uin.read(1)
        # bits per pixel
        BPP = uin.read(1)

        run = uin.read(1)

        Z = uin.read(1)
        if len(WD3) == 0 or len(HD3) == 0 or len(BPP) == 0 or len(run) == 0 or len(Z) == 0:
            break

        if ord(Z) != ord('Z'):
            print("OD: Unexpected input (Z)")
            sys.stdout.flush()
            continue

        if ord(run) == 0:
            break

        width = ord(WD3) * 8
        height = ord(HD3) * 8
        bpp = ord(BPP)
        size = int((width * height * bpp) / 8)
        print("OD: reading", size, "bytes")
        sys.stdout.flush()

        pixels = uin.read(size)
        bytes_read = len(pixels)
        while bytes_read < size:
            pixels += uin.read(size - bytes_read)
            bytes_read = len(pixels)
        
        print("OD: read:", len(pixels))
        print("OD: ready")
        sys.stdout.flush()

    print('OD: Exiting')
    sys.stdout.flush()

if __name__ == "__main__":
   main(sys.argv[1:])
