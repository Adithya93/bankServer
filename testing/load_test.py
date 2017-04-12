import concurrent.futures
import urllib.request
import func_test
import sys

CONC_LEVELS = [1, 3, 5, 10]
LOAD_LEVELS = [10, 20, 30, 40]

def load_test(host, load, conc):
    with concurrent.futures.ThreadPoolExecutor(max_workers=conc) as executor:
        futures = []
        results = [0, 0, 0, 0]
        completed = 0
        for i in range(conc):
            futures.append(executor.submit(func_test.run_tests, host, load, True))
        print("Submitted futures!")
        for i in range(conc):
            try:
                data = futures[i].result()
            except Exception as e:
                print("Ran into exception: ", e)
            else:
                print("Result from future " + str(i) + " : " + str(data))
                completed += 1
                for j in range(len(data)):
                    results[j] += data[j]
        if completed > 0 :
            avg_results = list(map(lambda x : x/completed, results))
            return avg_results
        return results


def printLoadTestResult(res, conc_level, load_level):
    print("Latency Results for " + str(conc_level) + " concurrent loads of " + str(load_level) + " requests each: " + str(res))

def do_load_tests(host):
    print("COMMENCING LOAD TEST")
    all_results = []
    for conc_level in CONC_LEVELS:
        conc_results = []
        for load_level in LOAD_LEVELS:
            next_result = load_test(host, load_level, conc_level)
            printLoadTestResult(next_result, conc_level, load_level)
            conc_results.append(next_result)
        all_results.append(conc_results)
    print("RESULTS OF LOAD TEST:")
    for conc_result_index in range(len(all_results)) :
        conc_result = all_results[conc_result_index]
        for load_result_index in range(len(conc_result)) :
            load_result = conc_result[load_result_index]
            printLoadTestResult(load_result, CONC_LEVELS[conc_result_index], LOAD_LEVELS[load_result_index])
    print("END OF LOAD TEST")


if __name__ == "__main__":
    numReqs = 10
    reset = True
    if len(sys.argv) < 2:
        print("Usage: python3 load_test.py <hostname>")
        sys.exit(1)
    HOST = sys.argv[1]
    do_load_tests(HOST)

