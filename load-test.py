# server to test on
import concurrent.futures
import cProfile
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
import gentest
import threading

lockreq = threading.Lock()
locktimeout = threading.Lock()

from enum import Enum

from lxml import etree

# Infra globals
RESPONSE_FILE_DIR = "./responses"  # TODO

# TODO swarm size stuff, use dir instead of single file, checkResults, use threadpool
# SWARMLET_INTERVAL = [0.1, 0.05, 0.01, 0.005, 0.001]
# SWARM_SIZES = [100] # ,10000,50000,100000]
SWARMLET_INTERVAL = [1]  # TODO
SWARM_SIZES = [1]  # ,10000,50000,100000]
SOCKET_TIMEOUT = 15  # seconds
MAX_WORKERS = 2

# diffTrees settings
DIFF_FILE = "./diff.txt"
INNER_TAGS = ["created", "transferred"]
FLOAT_REGEX = re.compile(r"[-+]?\d*\.\d+|\d+")


class LoadTest():

    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.requests = 0
        self.timeouts = 0

    def increment_requests(self):
        with lockreq:
            self.requests += 1

    def increment_timeout(self):
        with locktimeout:
            self.timeouts += 1

    def launch_swarmling(self, t_fname, res_fname, save_response=False):
        self.increment_requests();
        tfpath = os.path.join(gentest.TEST_FILE_DIR, t_fname)
        with open(tfpath, "rb") as tfile:
            # establish connection
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.settimeout(SOCKET_TIMEOUT)
                    s.connect((self.host, self.port))
                    l = tfile.read()  # TODO is this needed
                    tfile_size = len(l)
                    # print(l)
                    # send request to server
                    print("sending request of size of {0}".format(tfile_size))
                    s.sendall(struct.pack("!Q", tfile_size))
                    s.sendall(l)



                    # read response from serve
                    res_read_size = s.recv(8)
                    res_read_size_unpacked = struct.unpack(
                        "!Q", res_read_size)[0]
                    print("expecting response of size {0}".format(
                        res_read_size_unpacked))
                    res_xml = s.recv(res_read_size_unpacked)
                    if save_response:
                        with open(os.path.join(RESPONSE_FILE_DIR, res_fname), "wb") as f:
                            f.write(res_xml)
                    s.shutdown(socket.SHUT_WR)

            except socket.timeout:
                print("socket timed out.")
                self.increment_timeout();
                return

            except ConnectionRefusedError:
                print("connection refused.")
                self.increment_timeout();
                return

            except Exception as e:
                print(e)
                self.increment_timeout();

    def swarm(self):
        gentest.gentest()
        print("====== priming ======")
        self.launch_swarmling(gentest.PRIMER_FILE, "primer_response.xml", save_response=True)
        time.sleep(2)
        print("====== done ======")
        with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
            futures = []
            for t_fname in os.listdir(gentest.TEST_FILE_DIR):
                if t_fname[:len(gentest.TEST_FILE_PREFIX)] != gentest.TEST_FILE_PREFIX:
                    continue
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
            print("====== checking ======")
            time.sleep(2)
            self.launch_swarmling(gentest.CHECK_FILE, "check_response.xml", save_response=True)
            print("====== done =======")
            print("{0}/{1} completed".format(self.requests-self.timeouts,self.requests))

    def testCorrectness(self):
        self.launch_swarmling("query_test.xml","query_test_res.xml",save_response=True)
        self.launch_swarmling("drew_test.xml","drew_test_res.xml",save_response=True)
        self.launch_swarmling("nonexistent_account.xml","nonexistent_account_res.xml",save_response=True)


def tprint(x):
    print(etree.tostring(x, pretty_print=True))

def checkResults(self):
    with open("check_response.xml") as f:
        return

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
            found = False
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
    # cProfile.run('l.testCorrectness()')
    l.testCorrectness();
    # l.swarm()
    # keep result checking off critical path
    l.checkResults()
