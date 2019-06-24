import sys
import argparse

from . import *

def main(argv=None):
    parser = argparse.ArgumentParser(description='Prints transformed source.')
    parser.add_argument('filename')
    args = parser.parse_args(argv)

    with open(args.filename, 'rb') as f:
        text, _ = decode(f.read())
    getattr(sys.stdout, 'buffer', sys.stdout).write(text.encode('UTF-8'))


if __name__ == '__main__':
    exit(main())
