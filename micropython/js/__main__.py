import os
import sys
from functools import partial
from http.server import *

import argparse

try:
    import ssl
except:
    ssl = False

def test(HandlerClass=BaseHTTPRequestHandler, ServerClass=ThreadingHTTPServer, protocol="HTTP/1.0", port=8000, bind=""):
    """Test the HTTP request handler class.

    This runs an HTTP server on port 8000 (or the port argument).

    """
    server_address = (bind, port)

    HandlerClass.protocol_version = protocol


    with ServerClass(server_address, HandlerClass) as httpd:
        sa = httpd.socket.getsockname()


        if ssl:
            httpd.socket = ssl.wrap_socket (httpd.socket, keyfile='key.pem', certfile='server.pem', server_side=True)

        serve_message = "Serving HTTP on {host} port {port} (http://{host}:{port}/) ..."
        print(serve_message.format(host=sa[0], port=sa[1]))
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nKeyboard interrupt received, exiting.")
            sys.exit(0)


parser = argparse.ArgumentParser()

ROOT = os.path.dirname(os.path.dirname(__file__))

print("\nServing micropython files from [%s]\n\nwith no security/performance in mind, i'm just a test tool : don't rely on me" % ROOT)

parser.add_argument(
    "--bind", "-b", default="", metavar="ADDRESS", help="Specify alternate bind address " "[default: all interfaces]"
)
parser.add_argument("--directory", "-d", default=ROOT, help="Specify alternative directory " "[default:%s]" % ROOT)
parser.add_argument("port", action="store", default=8000, type=int, nargs="?", help="Specify alternate port [default: 8000]")

args = parser.parse_args()

handler_class = partial(SimpleHTTPRequestHandler, directory=args.directory)

if not ".wasm" in SimpleHTTPRequestHandler.extensions_map:
    print("WARNING: wasm mimetype unsupported on that system, trying to correct", file=sys.stderr)
    SimpleHTTPRequestHandler.extensions_map[".wasm"] = "application/wasm"

test(HandlerClass=handler_class, port=args.port, bind=args.bind)

