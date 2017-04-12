# prime DONE
# balance query
# transfer DONE
# create
# check DONE
# checkres DONE
# float for transfer values?

import os
import random
import string
from lxml import etree

NUMBERS = ['1','2','3','4','5','6','7','8','9','0']
LETTERS = list(string.ascii_lowercase)
MAX_TXNS_PER_FILE = 2
TAG_NUMBER_DISTRIBUTION = [0,0,0,0,0,1,1,1,2,2];
MAX_ACCT_BALANCE = 100
MAX_QUERIES_PER_FILE = 10
MAX_BAL_CHECK_PER_FILE = 5
NUM_ACCTS = 100
NUM_FILES = 10

TEST_FILE_DIR = "./testfiles"
TEST_FILE_PREFIX = "x"
PRIMER_FILE = "primer.xml"
CHECK_FILE = "check.xml"

class GenTest():

    def __init__(self):
        self.accounts_balances = {}

    def genPrime(self, fname):
        with open(fname, "wb") as f:
            # xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            root = etree.Element("transactions", reset="true")
            for i in range(NUM_ACCTS):
                accountname = "".join([random.choice(NUMBERS)
                                       for i in range(random.randint(8, 12))]) + str(i)
                # this might actually generate dudplicates
                balance = random.randint(0, MAX_ACCT_BALANCE)
                self.accounts_balances[accountname] = balance

                create = etree.Element("create", ref=str(i))
                root.append(create)

                account = etree.Element("account")
                account.text = accountname
                bal = etree.Element("balance")
                bal.text = str(balance)
                create.append(account)
                create.append(bal)

            f.write(
                etree.tostring(
                    root,
                    pretty_print=True,
                    xml_declaration=True,
                    encoding="UTF-8"))
            # print(self.accounts_balances.keys())
            self.accounts = list(self.accounts_balances.keys())

    def genCheck(self, fname):
        with open(fname, "wb") as f:
            root = etree.Element("transactions")
            for index, acct_name in enumerate(self.accounts):
                balance = etree.Element("balance", ref=acct_name)
                account = etree.Element("account")
                account.text = acct_name
                balance.append(account)
                root.append(balance)
            f.write(
                etree.tostring(
                    root,
                    pretty_print=True,
                    xml_declaration=True,
                    encoding="UTF-8"))

    def genCheckResult(self, fname):
        with open(fname, "wb") as f:
            root = etree.Element("results")
            for acct_name, bal in self.accounts_balances.items():
                success = etree.Element("success", ref=acct_name)
                success.text = str(bal)
                root.append(success)
            f.write(
                etree.tostring(
                    root,
                    pretty_print=True,
                    xml_declaration=True,
                    encoding="UTF-8"))

    def genTxns(self, num_files, ddir, name_prefix):
        for i in range(num_files):
            root = etree.Element("transactions")
            with open(os.path.join(ddir, name_prefix + str(i) + ".xml"), "wb") as f:
                num_txns = random.randint(1, MAX_TXNS_PER_FILE)
                for j in range(num_txns):
                    # pick 2 random accounts
                    account1 = random.choice(self.accounts)
                    balance1 = self.accounts_balances[account1]
                    account2 = random.choice(self.accounts)
                    balance2 = self.accounts_balances[account1]
                    # make sure they are not the same account
                    while account1 == account2:
                        account1 = random.choice(self.accounts)
                        balance1 = self.accounts_balances[account1]
                    transfer1_to_2 = random.randint(0, balance1)
                    # TODO divide by four for safety since we don't know order
                    # of execution?

                    transfer = etree.Element("transfer", ref=str(j))
                    to = etree.Element("to")
                    to.text = account2
                    fromm = etree.Element("from")
                    fromm.text = account1
                    amount = etree.Element("amount")
                    amount.text = str(transfer1_to_2)
                    num_tags = random.choice(TAG_NUMBER_DISTRIBUTION)
                    for k in range(num_tags):
                        tag = etree.Element("tag")
                        tag.text = "".join([random.choice(LETTERS)
                                       for i in range(2)])
                        transfer.append(tag);
                    transfer.append(fromm)
                    transfer.append(amount)
                    transfer.append(to)
                    root.append(transfer)

                    # track changes locally
                    self.accounts_balances[account1] -= transfer1_to_2
                    self.accounts_balances[account2] += transfer1_to_2

                f.write(
                    etree.tostring(
                        root,
                        pretty_print=True,
                        xml_declaration=True,
                        encoding="UTF-8"))
        print(self.accounts_balances)

def gentest():
    gt = GenTest()
    gt.genPrime(os.path.join(TEST_FILE_DIR,PRIMER_FILE))
    gt.genTxns(NUM_FILES, TEST_FILE_DIR, TEST_FILE_PREFIX)
    gt.genCheck(os.path.join(TEST_FILE_DIR,CHECK_FILE))
    gt.genCheckResult("./testfiles/check_expected_result.xml")

if __name__ == "__main__":
    gentest()
