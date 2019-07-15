import os
import sys
from functools import partial
from http.server import *

import argparse


def test_server(HandlerClass=BaseHTTPRequestHandler, ServerClass=ThreadingHTTPServer, protocol="HTTP/1.0", port=8000, bind="", ssl=False):
    """Test the HTTP request handler class.

    This runs an HTTP server on port 8000 (or the port argument).

    """

    server_address = (bind, port)

    HandlerClass.protocol_version = protocol

    with ServerClass(server_address, HandlerClass) as httpd:
        sa = httpd.socket.getsockname()


        if ssl:
            try:
                httpd.socket = modssl.wrap_socket (httpd.socket, keyfile='key.pem', certfile='server.pem', server_side=True)
            except Exception as e:
                print("can't start ssl",e)
                print("maybe 'openssl req -new -x509 -keyout key.pem -out server.pem -days 3650 -nodes'")
                ssl=False

        if ssl:
            serve_message = "Serving HTTPS on {host} port {port} (https://{host}:{port}/) ..."
        else:
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

parser.add_argument("--ssl",default=False,help="enable ssl with server.pem and key.pem")



parser.add_argument("port", action="store", default=8000, type=int, nargs="?", help="Specify alternate port [default: 8000]")

args = parser.parse_args()

ssl = args.ssl

if ssl:
    try:
        import ssl as modssl
        ssl = True
    except:
        print("Faulty ssl support")
        ssl = False
else:
    print("Not using SSL")

handler_class = partial(SimpleHTTPRequestHandler, directory=args.directory)

if not ".wasm" in SimpleHTTPRequestHandler.extensions_map:
    print("WARNING: wasm mimetype unsupported on that system, trying to correct", file=sys.stderr)
    SimpleHTTPRequestHandler.extensions_map[".wasm"] = "application/wasm"

test_server(HandlerClass=handler_class, port=args.port, bind=args.bind, ssl=ssl)

