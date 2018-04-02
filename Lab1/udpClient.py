import socket
import sys
import struct
import binascii
import time

# Here argument 1 is the hostname(www.website.com)
# And argument 2 is the port number
# Argument 3 is the operation to conduct (5, 80, 10)
# Argument 4 is the string used for said operation

if len(sys.argv) < 5:
	print "you must input argument hostname, followed by the port#, followed by an operation number (5,80,10), then a string!"
	sys.exit()

print "sys.argv[1] " + sys.argv[1]
print "socket.gethostbyname(sys.argv[1]) " + socket.gethostbyname(sys.argv[1])

BUFFER_SIZE = 256
UDP_IP = socket.gethostbyname(sys.argv[1])
UDP_PORT = int(sys.argv[2])
operation = int(sys.argv[3])
strArg = ' '.join(sys.argv[4:]) # joins the string arguments with spaces

startTime = time.time() # The start time

sock = socket.socket(socket.AF_INET, # Internet 
	socket.SOCK_DGRAM) # UDP

requestID = 1;
# Choose from operations
if operation == 10 or operation == 5 or operation == 80:
	
	# We must pack each value into byte size
	TML = len(strArg) + 3 #variable length + TML + requestID + Op
	packet = struct.pack("!BBB" + str(len(strArg)) + "s", TML, requestID,operation, strArg)
	sock.sendto(packet, (UDP_IP, UDP_PORT)) # Sends the packet
	data = sock.recvfrom(BUFFER_SIZE)	# waits for a reply
	
	#print "data: " + str(data[0])
	#print "type(data[0])" + str(type(data[0]))
	
	#print "type(data) " + str(type(data))
	realData = data[0];
	
	dataArray = list(data)
	#print "len(data) " + str(len(data))
	RTML = ord(realData[0])
	#print "TML: " + str(RTML)
	RrequestID = ord(realData[1])
	print "TML: " + str(RTML)
	
	print "RrequestID: " + str(RrequestID)

	print "Returned answer is: " + str(realData[2:RTML])
	print "len of data is :" + str(len(realData))
	
	requestID = requestID + 1;
	
else: 
	print "Please choose only from number 5, 80,or 10"

print "Elasped time: " + str(time.time() - startTime)