#!/usr/bin/env python3

import sys
import serial

if __name__ == '__main__':
    if len(sys.argv) < 4:
        sys.stderr.write(f'Usage: {sys.argv[0]} <tty> <passwd> <pwdid>\n')
        sys.exit(1)

    _, tty, pwdfile, pwdi = sys.argv
    pwdi = int(pwdi)

    with open(pwdfile, 'r', encoding='utf-8') as f:
        code = f.readlines()[pwdi].strip()

    ser = serial.Serial(tty, 9600)
    while True:
        line = ser.readline().decode('ascii').strip()
        if line.startswith('Y'):
            print('Heslo je:', code)
            sys.exit(0)
