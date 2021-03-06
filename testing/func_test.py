import concurrent.futures
import urllib.request
import socket
import sys
import random
import time
import struct
import func_test_xml_helper as xml_helper


HOST = 'localhost'
PORT = 3000
MAX_ACCOUNT_NUM = 999999
MAX_REF_VAL = 99999
MAX_BALANCE_VAL = 999999
TAG_PROBABILITY = 0.5
#SMALL_TRANSFER_LIMIT = 500000

#LARGE_TRANSFER_LIMIT = 105
LARGE_TRANSFER_LOWER_LIMIT = 100

FUNC_TEST_LOWER_LIMIT = 500000
FUNC_TEST_UPPER_LIMIT = 50000

LOAD_TEST_LOWER_LIMIT = 999000
LOAD_TEST_UPPER_LIMIT = 1000

FUNC_TEST_OR_TUP = (600000, 900000, 200000)
FUNC_TEST_AND_TUP = (300000, 900000, 999999)

LOAD_TEST_OR_TUP = (999000, 1000, 5000)
LOAD_TEST_AND_TUP = (800000, 10000, 20000)

# Tests to run

## CREATE

# Creating with reset = true causes subsequent BALANCE requests to return the latest values <DONE>
# Creating with reset = false for existing account <DONE>

## BALANCE

# Correct balances returned for existing account, verify together with create <DONE>
# Error returned for non-existent account (how to know for sure that it doesn't exist?) <SKIPPED>

## TRANSFER

# Transfer from non-existent account fails with error message <SKIPPED>
# Transfer to non-existent account fails with error message <SKIPPED>
# Transfer from account of amount more positive than FROM balance fails <DONE>
# Transfer to account of amount more negative than TO balance fails <DONE>
# Otherwise, transfer changes both accounts correctly <DONE>

## QUERY
# Simple relational operator <DONE>
# Simple logical operator <DONE>
# Nested Operator
# Empty NOT returns empty set <DONE>
# Empty OR returns empty set <DONE>
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


def makeTransferOp(fromAcc, to, amount, ref, tags = None):
    base = "<transfer ref=\"" + ref + "\">\n\t" + "<from>" + str(fromAcc) + "</from>\n\t" + "<to>" + str(to) + "</to>\n\t" + "<amount>" + str(amount) + "</amount>\n\t"  
    if tags :
        base += "".join(list(map(lambda x : "<tag>" + x + "</tag>", tags)))
    return base + "</transfer>"

def makeCreateOps(res, opList):
    opStrs = "".join(list(map(lambda x : makeCreateOp(x[0], x[1], x[2]), opList)))
    print("All create ops: " + opStrs)
    return opStrs

def makeBalanceOps(opsList):
    opStrs = "".join(list(map(lambda x : makeBalanceOp(x[0], x[1]), opsList)))
    print("All balance ops: " + opStrs)
    return opStrs

def makeTransferOps(opsList):
    opStrs = "".join(list(map(lambda x : makeTransferOp(x[0], x[1], x[2], x[3], x[4]), opsList)))
    print("All transfer ops: " + opStrs)
    return opStrs

# Wrap existing queryString with NOT
def makeNotQueryOp(query):
    return "<not>" + query + "</not>" 

# Wrap 2 existing queryStrings with OR
def makeOrQueryOp(queryList):
    opening = "<or>\n\t"
    closing = "\n</or>"
    return opening + "\n\t".join(queryList) + "\n" + closing

def makeAndQueryOp(queryList):
    opening = "<and>\n\t"
    closing = "\n</and>"
    return opening + "\n\t".join(queryList) + "\n" + closing

def makeGreaterQueryOp(attr, gauge):
    return "<greater " + attr + "=" + "\"" + str(gauge) + "\"/>"

def makeLesserQueryOp(attr, gauge):
    return "<less " + attr + "=" + "\"" + str(gauge) + "\"/>"

def makeEqualQueryOp(attr, gauge):
    return "<equals " + attr + "=" + "\"" + str(gauge) + "\"/>"

# Add wrapping-top-level-query tag to query
def wrapQuery(query, ref):
    return "<query ref=\"" + ref + "\">\n\t" + query + "\n</query>"

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
    refs.add(nextRef)
    return nextRef

def genAcc(accs):
    nextAcc = random.randrange(MAX_ACCOUNT_NUM)
    while nextAcc in accs:
        nextAcc = random.randrange(MAX_ACCOUNT_NUM)
    accs.add(nextAcc)
    return nextAcc

def get_create_responses(res, numReqs, use_old = None, old_accs = None):
    opsList = []
    accounts = {}
    refs = set([])
    accs = set(old_accs) if old_accs else set([]) 
    if use_old :
        print("Reusing old map!")
        accounts = use_old
        opsList = [(accounts[ref][0], ref, random.randrange(MAX_BALANCE_VAL)) for ref in accounts]
    else:
        for i in range(numReqs):
            nextRef = genRef(refs) # will guarantee uniqueness and update refs
            nextAcc = genAcc(accs) # same here
            queryTup = (nextAcc, nextRef, random.randrange(MAX_BALANCE_VAL)) 
            print("queryTup: " + str(queryTup))
            opsList.append(queryTup)
            accounts[queryTup[1]] = (queryTup[0], queryTup[2])
    createReqStr = makeCreateOps(res, opsList)
    body = wrapBody(createReqStr, res)
    print("Entire request: ")
    print(body)
    createResults, latency = sendRequest(body)
    return (createResults, accounts, latency) # tuple of (response string, map of refs to (account, balance))

# Make create reqs, wait for the response, then make balance reqs, and 
def test_create_balance(res, numReqs):
    createResults, accounts, create_latency = get_create_responses(res, numReqs)
    print("About to verify creation with balance requests")
    balanceRequest = genBalanceReqs(accounts)
    balanceResults, latency = sendRequest(balanceRequest)
    balanceResultsString = balanceResults
    print("Balance results: ")
    print(balanceResultsString)
    # call function which parses xml to return map of account to balance
    response_xml_root = parseResponse(balanceResultsString)
    # for every key in returned map, compare value of key with value in accounts map
    # print successes and failures
    failures = verifyBalances(accounts, response_xml_root)
    print("Completed test of CREATE & BALANCE with " + str(failures) + " failures out of " + str(numReqs) + " creates and balances")
    if failures == 0:
        print("UNIT TEST PASSED! :)")
    return (failures == 0, latency) #returns balance latency


def test_create_no_reset_fails(numReqs):
    createResults, accounts_map, create_latency = get_create_responses(True, numReqs)
    # repeat with same accounts, but with reset set to False; all requests must return <error>Already exists</error> (Text doesn't matter, tag does)
    repeatResults, same_accounts_map, create_latency_2 = get_create_responses(False, numReqs, accounts_map)
    # search repeatResults for error tags
    print("First map: ", accounts_map)
    print("Second map: ", same_accounts_map)
    root = parseResponse(repeatResults)
    failures = verifyCreateNoReset(same_accounts_map, root)
    print("Completed test of repeated CREATE with no reset with " + str(failures) + " failures out of " + str(numReqs) + " creates")
    if failures == 0:
        print("UNIT TEST PASSED! :)")
    return (failures == 0, (create_latency + create_latency_2)/2) #returns create latency


def transfer_insufficient_funds(numReqs, forFrom):
    # create accounts, retrieve map
    createResults, accounts_map, create_latency = get_create_responses(True, numReqs)
    otherCreateResults, other_accounts_map, create_latency_2 = get_create_responses(True, numReqs, None, list(map(lambda x : accounts_map[x][0], accounts_map.keys())))
    # for each (account, balance) make a transfer from account of balance + 5 (Won't overflow cos of cautious limit on MAX_BALANCE_VAL)
    opsList = []
    refs = set([])
    tags = set([])
    accounts_tups = [(list(accounts_map.keys())[i], list(other_accounts_map.keys())[i]) for i in range(len(accounts_map.keys()))]
    for (ref, other_ref) in accounts_tups :
        nextRef = genRef(refs)
        tag = [genRef(tags)] if random.random() < TAG_PROBABILITY else None # just for the sake of it
        if forFrom :
            queryTup = (accounts_map[ref][0], other_accounts_map[other_ref][0], accounts_map[ref][1] + 5, nextRef, tag)
        else:
            queryTup = (other_accounts_map[other_ref][0], accounts_map[ref][0], -1 * (accounts_map[ref][1] + 5), nextRef, tag)
        opsList.append(queryTup) 
        print("queryTup: " + str(queryTup))
    transferReqsStr = makeTransferOps(opsList)
    body = wrapBody(transferReqsStr, False)
    print("Entire request: ")
    print(body)
    transferResults, latency = sendRequest(body)
    print("Transfer results: ")
    print(transferResults)
    root = parseResponse(transferResults)
    testFailures = verifyTransferFailures(root, numReqs) # just searches for this many <error> tags
    if testFailures == 0:
        print("UNIT TEST PASSED! :)")
    return (testFailures == 0, latency) #returns transfer latency

def test_transfer_insufficient_funds_from_fails(numReqs):
    return transfer_insufficient_funds(numReqs, True) # returns transfer latency

def test_transfer_insufficient_funds_to_fails(numReqs):
    return transfer_insufficient_funds(numReqs, False) # returns transfer latency

# helper for below 2 functions
def transfer_valid(numReqs):
    # create 2 sets of accounts
    createResults, accounts_map, create_latency = get_create_responses(True, numReqs)
    otherCreateResults, other_accounts_map, create_latency_2 = get_create_responses(True, numReqs, None, list(map(lambda x : accounts_map[x][0], accounts_map.keys())))
    # transfer exactly the amount one has to the other
    # for each (account, balance) make a transfer from account of balance + 5 (Won't overflow cos of cautious limit on MAX_BALANCE_VAL)
    opsList = []
    refs = set([])
    tags = set([])
    exp_balances_from = {} # isolate for clearer debugging
    exp_balances_to = {}
    accounts_tups = [(list(accounts_map.keys())[i], list(other_accounts_map.keys())[i]) for i in range(len(accounts_map.keys()))]
    for (ref, other_ref) in accounts_tups :
        nextRef = genRef(refs)
        tag = [genRef(tags)] if random.random() < TAG_PROBABILITY else None # just for the sake of it
        queryTup = (accounts_map[ref][0], other_accounts_map[other_ref][0], accounts_map[ref][1], nextRef, tag)
        opsList.append(queryTup)
        exp_balances_from[ref] = (accounts_map[ref][0], 0)
        exp_balances_to[other_ref] = (other_accounts_map[other_ref][0], other_accounts_map[other_ref][1] + accounts_map[ref][1] )
        print("queryTup: " + str(queryTup))
    transferReqsStr = makeTransferOps(opsList)
    body = wrapBody(transferReqsStr, False)
    print("Entire request: ")
    print(body)
    transferResults, latency = sendRequest(body)
    print("Transfer results: ")
    print(transferResults)
    root = parseResponse(transferResults)
    failures = verifyTransferSuccesses(root, numReqs) # just searches for this many <error> tags
    return (failures, exp_balances_from, exp_balances_to, latency)

def test_transfer_valid_success(numReqs):
    failures, exp_balances_from, exp_balances_to, latency = transfer_valid(numReqs)#[0]
    if failures == 0:
        print("UNIT TEST PASSED! :)")
    return (failures == 0, latency) # returns transfer latency

def transfer_valid_balance(numReqs, forFrom):
    failures, exp_balances_from, exp_balances_to, transfer_latency = transfer_valid(numReqs)
    # get balances of all FROM accounts and verify correct 
    balanceRequest = genBalanceReqs(exp_balances_from) if forFrom else genBalanceReqs(exp_balances_to)
    chosen_map = exp_balances_from if forFrom else exp_balances_to
    balanceResults, balance_latency = sendRequest(balanceRequest)
    print("Balance results: ")
    print(balanceResults)
    # call function which parses xml to return map of account to balance
    response_xml_root = parseResponse(balanceResults)
    failures = verifyBalances(chosen_map, response_xml_root)
    if failures == 0:
        print("UNIT TEST PASSED! :)")
    return (failures == 0, balance_latency) # returns balance latency

def test_transfer_valid_balance_from(numReqs):
    return transfer_valid_balance(numReqs, True) # returns balance latency


def test_transfer_valid_balance_to(numReqs):
    return transfer_valid_balance(numReqs, False) # returns balance latency

def test_query_empty_not():
    empty_not_query_str = wrapQuery(makeNotQueryOp(""), genRef(set([])))
    body = wrapBody(empty_not_query_str, False)
    print("Query request: ")
    print(body)
    queryResults, latency = sendRequest(body)
    print("Query results: ")
    print(queryResults)
    root = parseResponse(queryResults)
    success = verifyEmptyResult(root)
    if success:
        print("UNIT TEST PASSED! :)")
    return (success, latency) # returns query latency

def test_query_empty_or():
    empty_or_query_str = wrapQuery(makeOrQueryOp(["", ""]), genRef(set([])))
    body = wrapBody(empty_or_query_str, False)
    print("Query request: ")
    print("body")
    queryResults, latency = sendRequest(body)
    print("Query results: ")
    print(queryResults)
    root = parseResponse(queryResults)
    success = verifyEmptyResult(root)
    if success:
        print("UNIT TEST PASSED! :)")
    return (success, latency) # returns query latency

def test_query_relational_greater(lower_limit):
    # choose a random amount, and check result to ensure all transfers fulfill criteria
    #transferAmount = random.randrange(SMALL_TRANSFER_LIMIT, 2 * SMALL_TRANSFER_LIMIT)
    transferAmount = random.randrange(lower_limit, 2 * lower_limit)
    greater_query_str = wrapQuery(makeGreaterQueryOp("amount", transferAmount), genRef(set([])))
    body = wrapBody(greater_query_str, False)
    print("Query request: ")
    print("body")
    queryResults, latency = sendRequest(body)
    print("Query results: ")
    print(queryResults)
    root = parseResponse(queryResults)
    success = verifyGreaterAttr(root, "amount", transferAmount)
    if success:
        print("UNIT TEST PASSED! :)")
    return (success, latency) # returns query latency

def test_query_relational_less(upper_limit):
    #transferAmount = random.randrange(LARGE_TRANSFER_LOWER_LIMIT, LARGE_TRANSFER_LIMIT)
    transferAmount = random.randrange(0, upper_limit)
    less_query_str = wrapQuery(makeLesserQueryOp("amount", transferAmount), genRef(set([])))
    body = wrapBody(less_query_str, False)
    print("Query request: ")
    print(body)
    queryResults, latency = sendRequest(body)
    print("Query results: ")
    print(queryResults)
    root = parseResponse(queryResults)
    success = verifyLesserAttr(root, "amount", transferAmount)
    if success:
        print("UNIT TEST PASSED! :)")
    return (success, latency) # returns query latency

def test_query_relational_equal():
    # Make a new transfer with a known FROM account to make sure it exists
    failures, exp_balances_from, exp_balances_to, transfer_latency = transfer_valid(1)
    account_from = exp_balances_from[list(exp_balances_from.keys())[0]][0]
    print("Account from: " + str(account_from))
    # Make query on equals
    equal_query_str = wrapQuery(makeEqualQueryOp("from", account_from), genRef(set([])))
    body = wrapBody(equal_query_str, False)
    print("Query request: ")
    print(body)
    queryResults, latency = sendRequest(body)
    print("Query results: ")
    print(queryResults)
    root = parseResponse(queryResults)
    success = verifyEqualAttr(root, "from", account_from)
    if success:
        print("UNIT TEST PASSED! :)")
    return (success, latency) # returns query latency

def test_query_logical_or(condList):
    queryList = []
    for (attr, rel, gauge) in condList:
        if rel == "greater":
            queryList.append(makeGreaterQueryOp(attr, gauge))
        elif rel == "less":
            queryList.append(makeLesserQueryOp(attr, gauge))
        else:
            queryList.append(makeEqualQueryOp(attr, gauge))
    or_query_str = wrapQuery(makeOrQueryOp(queryList), genRef(set([])))
    body = wrapBody(or_query_str, False)
    print("Or Query Request: ")
    print(body)
    queryResults, latency = sendRequest(body)
    print("Query Results:")
    print(queryResults)
    root = parseResponse(queryResults)
    success = verifyOr(root, condList)
    if success:
        print("UNIT TEST PASSED! :)")
    return (success, latency) # returns query latency

def test_query_logical_and(condList):
    queryList = []
    for (attr, rel, gauge) in condList:
        if rel == "greater":
            queryList.append(makeGreaterQueryOp(attr, gauge))
        elif rel == "less":
            queryList.append(makeLesserQueryOp(attr, gauge))
        else:
            queryList.append(makeEqualQueryOp(attr, gauge))
    and_query_str = wrapQuery(makeAndQueryOp(queryList), genRef(set([])))
    body = wrapBody(and_query_str, False)
    print("Or Query Request: ")
    print(body)
    queryResults, latency = sendRequest(body)
    print("Query Results:")
    print(queryResults)
    root = parseResponse(queryResults)
    success = verifyAnd(root, condList)
    if success:
        print("UNIT TEST PASSED! :)")
    return (success, latency) # returns query latency

# Come up with more robust/flexible way to test : possible if query class is written (if there is time)
def test_query_logical_not(lower_limit):
    transferAmount = random.randrange(lower_limit, 2 * lower_limit)
    less_query_str = makeLesserQueryOp("amount", transferAmount)
    not_query_str = wrapQuery(makeNotQueryOp(less_query_str), genRef(set([])))
    body = wrapBody(not_query_str, False)
    print("Query request: ")
    print(body)
    queryResults, latency = sendRequest(body)
    print("Query results: ")
    print(queryResults)
    root = parseResponse(queryResults)
    success = verifyGreaterAttr(root, "amount", transferAmount) or verifyEqualAttr(root, "amount", transferAmount)
    if success:
        print("UNIT TEST PASSED! :)")
    else:
        print("UNIT TEST FAILED! :(")
    return (success, latency) # returns query latency

def parseResponse(response):
    return xml_helper.parse(response)

def genBalanceReqs(accounts):
    opsList = []
    for account in accounts:
        opsList.append((accounts[account][0], account))
    balanceReqStr = makeBalanceOps(opsList)
    body = wrapBody(balanceReqStr, True)
    print("Balance Request body: ")
    print(body)
    return body

def verifyAnd(root, condList):
    return xml_helper.checkAnd(root, condList)

def verifyOr(root, condList):
    return xml_helper.checkOr(root, condList)

def verifyEqualAttr(root, attr, gauge) :
    return xml_helper.checkEquals(root, attr, gauge)

def verifyLesserAttr(root, attr, gauge) :
    return xml_helper.checkLesser(root, attr, gauge)

def verifyGreaterAttr(root, attr, gauge) :
    return xml_helper.checkGreater(root, attr, gauge)

def verifyEmptyResult(response_root):
    return len(xml_helper.getQueryResults(response_root)) == 0

def verifyTransferSuccesses(response_root, exp_num_successes) :
    return exp_num_successes - xml_helper.countSuccessTags(response_root)

def verifyTransferFailures(response_root, exp_num_failures) :
    return exp_num_failures - xml_helper.countErrorTags(response_root)

def verifyCreateNoReset(accounts_map, response_root):
    return [xml_helper.verifyErrorFromRef(response_root, ref) for ref in accounts_map].count(False)

def verifyBalance(exp_value, root, ref):
    act_value = xml_helper.getBalanceFromRef(root, ref)
    if act_value != exp_value:
        print("Error! Discrepancy on ref " + ref + " between expected value of " + str(exp_value) + " and actual value of " + str(act_value))
        return False
    print("Test passed on ref " + ref)
    return True

def verifyBalances(expected_accounts_map, response_root):
    return [verifyBalance(expected_accounts_map[ref][1], response_root, ref) for ref in expected_accounts_map].count(False)
    
def run_tests(host, numReqs=10, load_test=False):
    lower_limit = None
    upper_limit = None
    or_tup = None
    and_tup = None
    if load_test :
        lower_limit = LOAD_TEST_LOWER_LIMIT
        upper_limit = LOAD_TEST_UPPER_LIMIT
        or_tup = LOAD_TEST_OR_TUP
        and_tup = LOAD_TEST_AND_TUP

    else :
        lower_limit = FUNC_TEST_LOWER_LIMIT
        upper_limit = FUNC_TEST_UPPER_LIMIT
        or_tup = FUNC_TEST_OR_TUP
        and_tup = FUNC_TEST_AND_TUP

    HOST = host
    print("Host is now",HOST)
    results = []
    results.append(test_create_balance(True, numReqs)) # balance latency for numReqs
    results.append(test_create_no_reset_fails(numReqs)) # create latency for numReqs
    results.append(test_transfer_insufficient_funds_from_fails(numReqs)) # transfer latency for numReqs
    results.append(test_transfer_insufficient_funds_to_fails(numReqs)) # transfer latency for numReqs
    results.append(test_transfer_valid_success(numReqs)) # transfer latency for numReqs
    results.append(test_transfer_valid_balance_from(numReqs)) # balance latency for numReqs
    results.append(test_transfer_valid_balance_to(numReqs)) # balance latency for numReqs
    results.append(test_query_empty_not()) # query latency for 1 req
    results.append(test_query_empty_or()) # query latency for 1 req
    results.append(test_query_relational_greater(lower_limit)) # query latency for 1 req
    results.append(test_query_relational_less(upper_limit)) # query latency for 1 req
    results.append(test_query_relational_equal()) # query latency for 1 req
    results.append(test_query_logical_or([("amount", "greater", or_tup[0]), ("amount", "less", or_tup[1]), ("to", "less", or_tup[2])])) # query latency for 1 req
    results.append(test_query_logical_and([("amount", "greater", and_tup[0]), ("amount", "less", and_tup[1]), ("to", "less", and_tup[2])])) # query latency for 1 req
    results.append(test_query_logical_not(lower_limit)) # query latency for 1 req
    successes = list(map(lambda x : x[0], results)).count(True)
    timings = list(map(lambda x: x[1], results))
    avg_create_latency = (timings[1])/numReqs
    avg_balance_latency = (timings[0] + timings[5] + timings[6])/(3 * numReqs)
    avg_transfer_latency = (timings[2] + timings[3] + timings[4])/(3 * numReqs)
    avg_query_latency = sum(timings[i] for i in range(7, len(timings)))/((len(timings) - 7) * numReqs)
    timings_tup = (avg_create_latency, avg_balance_latency, avg_transfer_latency, avg_query_latency)
    #timings_list = [1000 * timings_tup[i] for i in range(len(timings_tup))]
    timings_list = list(map(lambda x : x * 1000, timings_tup))
    print("Average Timings: CREATE: " + str(timings_list[0]) + "ms; BALANCE: " + str(timings_list[1]) + "ms; TRANSFER: " + str(timings_list[2]) + "ms; QUERY: " + str(timings_list[3]) + "ms")
    failures = results.count(False)
    print("COMPLETED " + str(len(results)) + " TESTS WITH " + str(successes) + " SUCCESSES and " + str(failures) + " FAILURES.")
    return timings_list

def sendRequest(body):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        #s.connect((socket.gethostbyaddr(HOST), PORT))
        s.sendall(addHeader(body))
        # start timing
        start_time = time.time()
        body_len_bytes = s.recv(8)
        end_time = time.time() # Eventually decided to cap the timing here to exclude the time for receiving long responses on client socket
        print("Byte header received: ", body_len_bytes)
        body_len = struct.unpack('>Q', body_len_bytes)[0]
        print("Body length mentioned in header: ", body_len)
        recvd = 0
        data = b''
        time_taken = end_time - start_time
        recvd_data = None
        while recvd < body_len:
            #start_time = time.time()
            recvd_data = s.recv(body_len - recvd)
            #end_time = time.time()
            #time_taken += end_time - start_time
            recvd += len(recvd_data)
            data += recvd_data
        #end_time = time.time()
        # stop timing
        response = repr(data)
        print("Length of receieved data: ", str(len(response)))
        print('Received', response)
        return (bytes.decode(data), time_taken)

if __name__ == "__main__":
    numReqs = 10
    reset = True
    if len(sys.argv) < 2:
        print("Usage: python3 func_test.py <hostname> [numReqs=100] [reset=True]")
        sys.exit(1)
    HOST = sys.argv[1]
    if len(sys.argv) > 2 :
        numReqs = int(sys.argv[2])
        if len(sys.argv) > 3:
            reset = True if sys.argv[3].lower() == 't' else False
    run_tests(HOST, numReqs)
