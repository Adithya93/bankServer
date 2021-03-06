import xml.etree.ElementTree as ET

def parse(response):
	return ET.fromstring(response)

def getValueFromRef(parent, ref):
	foundTag = parent.findall(".//*[@ref='" + ref + "']")
	if len(foundTag) > 0:	
		foundVal = foundTag[0].text
		print("Found text: " + foundVal)
		return foundVal
	print("Ref not found!")
	return None

def getTagFromRef(parent, ref):
	print("Searching for ref: " + ref)
	foundTag = parent.findall(".//*[@ref=\"" + ref + "\"]")
	if len(foundTag) > 0:
		tagResult = foundTag[0].tag
		print("Tag name: " + tagResult)
		return tagResult
	print("Ref not found!")
	return None

def getBalanceFromRef(root, ref):
	foundStr = getValueFromRef(root, ref)
	if foundStr == None:
		return None
	return float(foundStr)

def verifyErrorFromRef(root, ref):
	foundTag = getTagFromRef(root, ref)
	if foundTag != "error":
		print("Error expected but not present!")
		return False
	print("Correct error-handling")
	return True

def countErrorTags(root):
	return len(root.findall("error"))

def countSuccessTags(root):
	return len(root.findall("success"))

def getQueryResults(root):
	return root.findall("./results/transfer")

def checkGreater(root, attr, gauge):
	transfers = root.findall("./results/transfer")
	for transfer in transfers:
		tested_attr = transfer.find(attr)
		if float(tested_attr.text) <= gauge :
			print("Failed on " + tested_attr.text + "; not greater than " + str(gauge))
			return False
		print("Passed on " + tested_attr.text)
	return True

def checkLesser(root, attr, gauge):
	transfers = root.findall("./results/transfer")
	for transfer in transfers:
		tested_attr = transfer.find(attr)
		if float(tested_attr.text) >= gauge :
			print("Failed on " + tested_attr.text + "; not lesser than " + str(gauge))
			return False
		print("Passed on " + tested_attr.text)
	return True

def checkEquals(root, attr, gauge):
	transfers = root.findall("./results/transfer")
	for transfer in transfers:
		tested_attr = transfer.find(attr)
		if float(tested_attr.text) != gauge :
			print("Failed on " + tested_attr.text + "; not equal to " + str(gauge))
			return False
		print("Passed on " + tested_attr.text)
	return True

# does not handle nested logicals yet
def checkOr(root, condList):
	transfers = root.findall("./results/transfer")
	for transfer in transfers:
		passed = False
		for (attr, rel, gauge) in condList:
			print("Searching for " + rel + ", " + attr + str(gauge))
			tested_attr = transfer.find(attr)
			if tested_attr == None: #missing attribute - error
				print("Transfer element is missing attribute " + attr)
				return False
			if rel == "greater" :
				if float(tested_attr.text) > gauge:  # the or is fulfilled for this transfer already
					print("transfer " + attr + " : " + tested_attr.text + " passed on " + rel + " : " + str(gauge))
					passed = True
					break
				#else:
				#	print("transfer " + attr + " : " + tested_attr.text + " does not fulfill " + rel + " : " + str(gauge))
			elif rel == "less" :
				if float(tested_attr.text) < gauge:
					print("transfer " + attr + " : " + tested_attr.text + " passed on " + rel + ": " + str(gauge))
					passed = True
					break
			elif rel == "equals" :
				if float(tested_attr.text) == gauge:
					print("transfer " + attr + " : " + tested_attr.text + " passed on " + rel + ": " + str(gauge))
					passed = True
					break
			print("transfer " + attr + " : " + tested_attr.text + " does not fulfill " + rel + " : " + str(gauge))
		if not passed:
			print("Transfer failed to fulfill any OR condition!")
			return False
		print("Transfer passed")
	return True

# does not handle nested logicals yet
def checkAnd(root, condList):
	transfers = root.findall("./results/transfer")
	for transfer in transfers:
		passed = True
		for (attr, rel, gauge) in condList:
			print("Searching for " + rel + ", " + attr + str(gauge))
			tested_attr = transfer.find(attr)
			if tested_attr == None: #missing attribute - error
				print("Transfer element is missing attribute " + attr)
				return False
			if rel == "greater" :
				if float(tested_attr.text) <= gauge:  # the or is fulfilled for this transfer already
					print("transfer " + attr + " : " + tested_attr.text + " eliminated by " + rel + " : " + str(gauge))
					return False
			elif rel == "less" :
				if float(tested_attr.text) >= gauge:
					print("transfer " + attr + " : " + tested_attr.text + " eliminated by " + rel + ": " + str(gauge))
					return False
			elif rel == "equals" :
				if float(tested_attr.text) != gauge:
					print("transfer " + attr + " : " + tested_attr.text + " eliminated by " + rel + ": " + str(gauge))
					return False
			print("transfer " + attr + " : " + tested_attr.text + " fulfills " + rel + " : " + str(gauge))
		print("Transfer passed")
	return True

'''
tree = ET.parse('errorTest')
root = tree.getroot()
checkGreater(root, "amount", 100)
getQueryResults(root)
print("Errors found:", countErrorTags(root))
verifyErrorFromRef(root, "c1")
tree = ET.parse('xmlTest')
root = tree.getroot()
print("Root tag: " + root.tag)
for child in root:
	print("Child tag: ", child.tag, "; Child attribute: ", child.attrib)
queryResults = root.findall('results')
for queryResult in queryResults:
	print("query result", str(queryResult))

refSearch = root.findall(".//*[@ref='xyz']")
for res in refSearch:
	print("Tag of desired ref: ", res.tag, "; attribute: ", res.attrib)
	print("Text of desired tag: " + res.text)

#getValueFromRef(root, "xyz")
getBalanceFromRef(root, "xyz")

'''