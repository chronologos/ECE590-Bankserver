# server to test on
import socket
import sys
import struct
import time
import select
import signal
import concurrent.futures
import os

RESPONSE_FILE_PATH = "./testfiles/test1r.xml"


class LoadtestServer():

    def __init__(self, host: str, port: str):
        self.host = host
        self.port = port
        self.count = 0
        return

    def listen(self):
        print("server: single thread started...")
        sock = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        sock.bind((self.host, 12345))
        # become a server socket
        sock.listen(5)
        signal.signal(signal.SIGINT, self.handler)

        while True:
            self.response_file = open(RESPONSE_FILE_PATH, "rb")
            self.size = os.path.getsize(RESPONSE_FILE_PATH)
            # establish connection
            clientSocket, addr = sock.accept()

            # get request from client
            read_size = clientSocket.recv(8)
            read_size_unpacked = struct.unpack("q", read_size)[0]
            # result is a tuple even if it contains one item
            print("server: got a connection from %s" % str(addr))
            print ("server: expecting xml file of size {0}".format(read_size_unpacked))
            xml = clientSocket.recv(read_size_unpacked)
            print (xml)
            self.count += 1
            time.sleep(1)

            # after calculations, respond to client
            print("server: replying with size {0} packet".format(self.size))
            clientSocket.sendall(struct.pack("q", self.size))
            l = self.response_file.read(self.size)  # TODO
            while (l):
                clientSocket.sendall(l)
                l = self.response_file.read(self.size)
            print("server: done with reply")
            self.response_file.close()
            clientSocket.close()

    def handler(self, signum, frame):
        print("server: exiting. count = " + str(self.count))
        exit()

if __name__ == "__main__":
    l = LoadtestServer(host="localhost", port="12345")
    l.listen()
    print ("listening")
