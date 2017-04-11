import concurrent.futures
import urllib.request
import socket
import sys
import random
#import xml.etree.ElementTree as ET
import func_test_xml_helper as xml_helper


HOST = 'localhost'
PORT = 3000
MAX_ACCOUNT_NUM = 999999
MAX_REF_VAL = 99999
MAX_BALANCE_VAL = 999999


# Tests to run

## CREATE

# Creating with reset = true causes subsequent BALANCE requests to return the latest values
# Creating with reset = false for existing account

## BALANCE

# Correct balances returned for existing account, verify together with create
# Error returned for non-existent account (how to know for sure that it doesn't exist?)

## TRANSFER

# Transfer from non-existent account fails with error message
# Transfer to non-existent account fails with error message
# Transfer from account of amount more positive than FROM balance fails
# Transfer to account of amount more negative than TO balance fails
# Otherwise, transfer changes both accounts correctly, sum is same

## QUERY
# Simple relational operator
# Simple logical operator
# Nested Operator
# Empty NOT returns empty set
# Empty AND contains all transfers issued during test

def addHeader(body):
    print("Body is " + body)
    headerBytes = (len(body)).to_bytes(8, byteorder='big')
    result = headerBytes + body.encode('utf-8')
    #print("Result is " + bytes.decode(result))
    return result

def makeCreateOp(account, ref, balance=None):
    balanceStr = ""
    if balance != None:
        balanceStr += "\t<balance>" + str(balance) + "</balance>\n"
    opStr = "<create ref=\"" + ref + "\">\n\t" + "<account>" + str(account) + "</account>\n" + balanceStr + "</create>" 
    print(opStr)
    return opStr
    

def makeBalanceOp(account, ref):
    return "<balance ref =\"" + ref + "\">\n\t" + "<account>" + str(account) + "</account>\n" + "</balance>"

def makeCreateOps(res, opList):
    #opStrs = ""
    #[opStrs += makeCreateOp(op[0], op[1], op[2]) for op in opList]
    '''
    for op in opList:
        opStrs += makeCreateOp(op[0], op[1], op[2])
    '''
    opStrs = "".join(list(map(lambda x : makeCreateOp(x[0], x[1], x[2]), opList)))
    print("All create ops: " + opStrs)
    return opStrs

def makeBalanceOps(opsList):
    opStrs = "".join(list(map(lambda x : makeBalanceOp(x[0], x[1]), opsList)))
    print("All balance ops: " + opStrs)
    return opStrs

# Add the top-level opening/closing tags
def wrapBody(body, res):
    resStr = "false"
    if res:
        resStr = "true"
    header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><transactions reset=\"" + resStr + "\">"
    footer = "</transactions>"
    return header + body + footer

# generate a random 2-digit hex string ref
def genRef(refs):
    nextRef = format(random.randrange(MAX_REF_VAL), 'x')
    while nextRef in refs:
        nextRef = format(random.randrange(MAX_REF_VAL), 'x')

    #while ((nextRef = format(random.randrange(MAX_REF_VAL), 'x')) in refs):
        # nothing
    refs.add(nextRef)
    return nextRef

def genAcc(accs):
    nextAcc = random.randrange(MAX_ACCOUNT_NUM)
    #while ((nextAcc = random.randrange(MAX_ACCOUNT_NUM)) in accs):
        #nothing
    while nextAcc in accs:
        nextAcc = random.randrange(MAX_ACCOUNT_NUM)
    accs.add(nextAcc)
    return nextAcc

# Make create reqs, wait for the response, then make balance reqs, and 
def testCreateAndBalance(res, numReqs):
    '''
    resStr = "false"
    if res:
        resStr = "true"
    header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><transactions reset=\"" + resStr + "\">"
    print("Header is " + header)
    footer = "</transactions>"
    '''
    opsList = []
    accounts = {}
    refs = set([])
    accs = set([])
    #createReqStr = ""
    for i in range(numReqs):
        nextRef = genRef(refs) # will guarantee uniqueness and update refs
        nextAcc = genAcc(accs)
        queryTup = (nextAcc, nextRef, random.randrange(MAX_BALANCE_VAL)) 
        print("queryTup: " + str(queryTup))
        opsList.append(queryTup)
        #accounts.append(queryTup[0])
        #accounts[queryTup[0]] = queryTup[2]
        accounts[queryTup[1]] = (queryTup[0], queryTup[2])
    #header += makeCreateOps(res, opsList)
    #header += footer
    createReqStr = makeCreateOps(res, opsList)
    body = wrapBody(createReqStr, res)
    print("Entire request: ")
    #print(header)
    print(body)
    #verifyResults(expected, sendRequest(header))
    #createResults = sendRequest(header)
    createResults = sendRequest(body)
    print("About to verify creation with balance requests")
    balanceRequest = genBalanceReqs(accounts)
    balanceResults = sendRequest(balanceRequest)
    #balanceResultsString = bytes.decode(balanceResults)
    balanceResultsString = balanceResults
    print("Balance results: ")
    print(balanceResultsString)
    # call function which parses xml to return map of account to balance
    response_xml_root = parseResponse(balanceResultsString)
    # for every key in returned map, compare value of key with value in accounts map
    # print successes and failures
    failures = verifyCreatesAndBalances(accounts, response_xml_root)
    print("Completed test of CREATE & BALANCE with " + str(failures) + " failures out of " + str(numReqs) + " creates and balances")

def parseResponse(response):
    return xml_helper.parse(response)

def genBalanceReqs(accounts):
    opsList = []
    for account in accounts:
        #opsList.append((accounts[account][0], genRef()))
        opsList.append((accounts[account][0], account))
    balanceReqStr = makeBalanceOps(opsList)
    body = wrapBody(balanceReqStr, True)
    print("Balance Request body: ")
    print(body)
    return body


def verifyBalance(exp_value, root, ref):
    act_value = xml_helper.getBalanceFromRef(root, ref)
    if act_value != exp_value:
        print("Error! Discrepancy on ref " + ref + " between expected value of " + str(exp_value) + " and actual value of " + str(act_value))
        return False
    print("Test passed on ref " + ref)
    return True
 
def verifyCreatesAndBalances(expected_accounts_map, response_root):
    return [verifyBalance(expected_accounts_map[ref][1], response_root, ref) for ref in expected_accounts_map].count(False)
    

def sendRequest(body):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        s.sendall(addHeader(body))
        data = s.recv(8192)
    response = repr(data)
    print('Received', response)
    return bytes.decode(data)
    #return response


testCreateAndBalance(True, 100)
