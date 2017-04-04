# server to test on
import concurrent.futures
import datetime
import os
import random
import re
import select
import signal
import socket
import struct
import sys
import time
from enum import Enum

from lxml import etree

# Infra globals
TEST_FILE_PATH = "./testfiles/test1.xml"  # TODO
RESPONSE_FILE_PATH = "./responses/res100.xml"
TEST_FILE_DIR = "./testfiles"  # TODO
RESPONSE_FILE_DIR = "./responses"  # TODO
CORRECT_ANSWER_DIR = "./ansfiles"
DIFF_FILE = "./diff.txt"


# TODO swarm size stuff, use dir instead of single file, checkResults, use threadpool
# SWARMLET_INTERVAL = [0.1, 0.05, 0.01, 0.005, 0.001]
# SWARM_SIZES = [100] # ,10000,50000,100000]
SWARMLET_INTERVAL = [1]  # TODO
SWARM_SIZES = [1]  # ,10000,50000,100000]
SOCKET_TIMEOUT = 5  # seconds

# Parsing globals
INNER_TAGS = ["created", "transferred"]
FLOAT_REGEX = re.compile(r"[-+]?\d*\.\d+|\d+")


class LoadTest():

    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.requests = 0
        self.timeouts = 0

    def launch_swarmling(self, t_fname, res_fname):
        self.requests += 1
        tfpath = os.path.join(TEST_FILE_DIR, t_fname)
        with open(tfpath, "rb") as tfile:
            tfile_size = os.path.getsize(tfpath)
            # establish connection
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.settimeout(SOCKET_TIMEOUT)
                    s.connect((self.host, self.port))

                    # send request to server
                    print("sending request of size of {0}".format(tfile_size))
                    s.sendall(struct.pack("q", tfile_size))

                    l = tfile.read(tfile_size)  # TODO is this needed
                    while (l):
                        s.sendall(l)
                        l = tfile.read(tfile_size)

                    # read response from serve
                    res_read_size = s.recv(8)
                    res_read_size_unpacked = struct.unpack(
                        "q", res_read_size)[0]
                    print("expecting response of size {0}".format(
                        res_read_size_unpacked))
                    res_xml = s.recv(res_read_size_unpacked)
                    with open(os.path.join(RESPONSE_FILE_DIR, res_fname), "wb") as f:
                        f.write(res_xml)
                    s.shutdown(socket.SHUT_WR)

            except socket.timeout:
                print("socket timed out.")
                self.timeouts += 1
                return

            except ConnectionRefusedError:
                print("connection refused.")
                self.timeouts += 1
                return

    def swarm(self):
        # signal.signal(signal.SIGINT, self.handler)
        # print(os.listdir(CORRECT_ANSWER_DIR))
        # print(os.listdir(RESPONSE_FILE_DIR))

        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
            futures = []
            for t_fname in os.listdir(TEST_FILE_DIR):
                t_number, _ = t_fname.split(".")
                number = t_number[4:]
                res_fname = "res{0}.xml".format(number)  # TODO
                # print(res_fname)
                futures.append(
                    executor.submit(
                        self.launch_swarmling,
                        t_fname,
                        res_fname))
            concurrent.futures.wait(futures)
            for future in futures:
                future.result()

        # try:
        #     for si in SWARMLET_INTERVAL:
        #         for ss in SWARM_SIZES:
        #             print(
        #                 "swarmlet interval {0}, swarm size {1}".format(
        #                     si, ss))
        #             for x in range(ss):
        #                 time.sleep(si)
        #                 self.launch_swarmling()
        # except ConnectionRefusedError:
        #     print("swarmer exiting. count = " + str(self.requests))
        #     exit()

    def checkResults(self):  # TODO
        print(
            "Loadtest complete. Total requests made: {0}, total requests dropped: {1} \n See diff.txt for correctness checks.".format(
                self.requests,
                self.timeouts))
        for res_fname in os.listdir(RESPONSE_FILE_DIR):
            res_number, _ = res_fname.split(".")
            number = res_number[3:]
            # assumption: responses called resXX.xml, where XX is test number.
            # ans_fname = "r{0}.xml".format(number) #TODO
            ans_fname = "r{0}.xml".format(number)  # TODO
            ans_tree = etree.parse(os.path.join(CORRECT_ANSWER_DIR, ans_fname))
            res_tree = etree.parse(os.path.join(RESPONSE_FILE_DIR, res_fname))
            diffTrees(number, res_tree, ans_tree)
        return

    # def handler(self, signum, frame):
    #     print("swarmer exiting. closing file. total count sent = " + str(self.requests))
    #     tfile.close()
    #     exit()


def tprint(x):
    print(etree.tostring(x, pretty_print=True))


def diffTrees(tnum, res, ans):
    # Compares two XML files and print differences to file.
    res_root = res.getroot()
    ans_root = ans.getroot()

    # os.remove(DIFF_FILE) # TODO
    with open(DIFF_FILE, "a") as f:
        f.write(
            "diffTrees output for test {0} at {1}\n".format(
                tnum, datetime.datetime.now()))
        if res_root.tag != "results":
            f.write(
                "root tag has to be '<results>' but got <{0}>\n".format(
                    res_root.tag))

        # tprint(root)
        ans_child_nochildren = [x for x in ans_root if len(x) == 0]
        ans_child_haschildren = [x for x in ans_root if len(x) > 0]
        res_child_nochildren = [x for x in res_root if len(x) == 0]
        res_child_haschildren = [x for x in res_root if len(x) > 0]
        for c in ans_child_nochildren:
            found:
                bool = False
            ans_tag, ans_ref, ans_text = c.tag, c.get("ref"), c.text
            # print("ans: tag: {0}, ref: {1}, text: {2}".format(ans_tag,ans_ref,ans_text))

            # find corresponding ref in res_root
            for c in res_child_nochildren:

                res_tag, res_ref, res_text = c.tag, c.get("ref"), c.text
                # print("match a-{0}, r-{1} text a-{2}, r-{3} refs".format(ans_text, res_text, ans_ref, res_ref))

                if (ans_ref == res_ref) and ans_ref is not None:
                    found = True

                    if ans_tag != res_tag:
                        f.write(
                            "<ref='{0}'> tag mismatch: got '{1}' but expected '{2}'\n".format(
                                ans_ref, res_tag, ans_tag))

                    elif ans_text != res_text and ans_tag == "success":
                        m1 = FLOAT_REGEX.fullmatch(ans_text)
                        m2 = FLOAT_REGEX.fullmatch(res_text)
                        if m1 and m2:
                            # print("two floats {0} {1}".format(res_text, ans_text))
                            continue
                        elif ans_text not in INNER_TAGS and res_text not in INNER_TAGS:
                            f.write(
                                "<ref='{0}'> text mismatch in <success>: got '{1}' but expected '{2}'\n".format(
                                    ans_ref, res_text, ans_text))

            if not found:
                f.write("<ref='{0}'> not found.\n".format(ans_ref))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: ./load-test portno")
    l = LoadTest(host=sys.argv[1], port=12345)
    l.swarm()
    # keep result checking off critical path
    l.checkResults()
