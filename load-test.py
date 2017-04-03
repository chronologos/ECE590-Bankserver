# server to test on
import socket
import sys
import struct
import time
import select
import signal
import time
import random
import os

TEST_FILE_PATH = "./testfiles/test1.xml"

# SWARMLET_INTERVAL = [0.1, 0.05, 0.01, 0.005, 0.001]
# SWARM_SIZES = [100] # ,10000,50000,100000]
SWARMLET_INTERVAL = [1]
SWARM_SIZES = [1]  # ,10000,50000,100000]
SOCKET_TIMEOUT = 5 # seconds

class LoadtestSwarm():

    def __init__(self, host: str, port: str):
        self.host = host
        self.port = port
        self.count = 0
        self.file = open(TEST_FILE_PATH, "rb")
        self.size = os.path.getsize(TEST_FILE_PATH)
        return

    def launch_swarmling(self):

        # establish connection
        self.sock = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(SOCKET_TIMEOUT)
        self.sock.connect((self.host, 12345))

        # send request to server
        print("sending request of size of {0}".format(self.size))
        self.sock.sendall(struct.pack("q", self.size))
        l = self.file.read(self.size)  # TODO
        while (l):
            self.sock.sendall(l)
            l = self.file.read(self.size)

        # read response from serve
        res_read_size = self.sock.recv(8)
        res_read_size_unpacked = struct.unpack("q", res_read_size)[0]
        print("expecting response of size {0}".format(res_read_size_unpacked))
        try:
            res_xml = self.sock.recv(res_read_size_unpacked)
        except socket.timeout:
            print("socket timed out.")
            return
        with open("test1res.xml", "wb") as f:
            f.write(res_xml)
        print(os.path.getsize("test1res.xml"))
        print (res_xml)
        self.count += 1
        self.sock.shutdown(socket.SHUT_WR)
        self.sock.close()
        self.count += 1

    def swarm(self):
        signal.signal(signal.SIGINT, self.handler)
        try:
            for si in SWARMLET_INTERVAL:
                for ss in SWARM_SIZES:
                    print(
                        "swarmlet interval {0}, swarm size {1}".format(
                            si, ss))
                    for x in range(ss):
                        time.sleep(si)
                        self.launch_swarmling()
        except ConnectionRefusedError:
            print("swarmer exiting. count = " + str(self.count))
            exit()

    def checkResults(self):
        return

    def handler(self, signum, frame):
        print("swarmer exiting. closing file. total count sent = " + str(self.count))
        self.file.close()
        exit()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: ./load-test portno")
    l = LoadtestSwarm(host=sys.argv[1], port=12345)
    l.swarm()
    l.checkResults()
