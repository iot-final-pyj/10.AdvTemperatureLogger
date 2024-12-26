from http.server import BaseHTTPRequestHandler, HTTPServer
import time
from urllib.parse import urlparse, parse_qsl

hostName = '0.0.0.0'
serverPort = 8000

class MyServer(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        qs = urlparse(self.path).query
        if len(qs) > 0:
            for q in parse_qsl(qs):
                if q[0] == "temperature":
                    print("temperature is" , q[1])
        self.wfile.write(bytes('{status:"ok"}', 'utf-8'))

    def do_POST(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        if self.path == '/':
            self.wfile.write(bytes('{"status":"ng"}', 'utf-8'))
        else:
            try:
                content_length = int(self.headers['Content-Length'])
                post_data = self.rfile.read(content_length)
                print(post_data)
                print(self.path)
                self.wfile.write(bytes('{"status":"ok"}', 'utf-8'))
                with open('.' + self.path, 'w') as f:
                    f.write(post_data.decode())
                    f.close()
            except BaseException:
                pass

webServer = HTTPServer((hostName, serverPort), MyServer)
print('Server started http://%s:%s' % (hostName, serverPort))

try:
    webServer.serve_forever()
except KeyboardInterrupt:
    pass

webServer.server_close()
print('Server stopped')
