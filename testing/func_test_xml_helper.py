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

def getBalanceFromRef(root, ref):
	foundStr = getValueFromRef(root, ref)
	if foundStr == None:
		return None
	return float(foundStr)




'''
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