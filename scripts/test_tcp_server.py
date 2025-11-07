import time
import socketserver

class MyTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        print ("[OK] Device Connected ! ")
        while True:
            print(f"Sending ...")
            self.request.sendall(b"t321t")
            time.sleep(1)

if __name__ == "__main__":
    HOST, PORT = "192.168.0.12", 2111

    print(f"TCP server started with HOST={HOST} and PORT={PORT}")
    print(f"Interrupt the program with Ctrl-C")
    print(f">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")

    with socketserver.TCPServer((HOST, PORT), MyTCPHandler) as server:
        # Activate the server; this will keep running until you
        # interrupt the program with Ctrl-C
        server.serve_forever()