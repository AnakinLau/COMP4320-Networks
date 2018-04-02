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

TCP_IP = socket.gethostbyname(sys.argv[1])
TCP_PORT = int(sys.argv[2])
BUFFER_SIZE = 256
MESSAGE = "Hello, Server!"

operation = int(sys.argv[3])
strArg = ' '.join(sys.argv[4:]) # joins the string arguments with spaces

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # create/setup socket

startTime = time.time() # The start time

s.connect((TCP_IP, TCP_PORT))	# initial connect
s.send(MESSAGE)
data = s.recv(BUFFER_SIZE)
print "data " + data

requestID = 1;

if operation == 10 or operation == 5 or operation == 80:
	
	# We must pack each value into byte size
	TML = len(strArg) + 3 #variable length + TML + requestID + Op
	packet = struct.pack("!BBB" + str(len(strArg)) + "s", TML, requestID,operation, strArg)
	s.send(packet) # Sends the packet
	data = s.recvfrom(BUFFER_SIZE)	# waits for a reply
	
	print "data: " + str(data)
	dataArray = list(data)

	RTML = ord(data[0])
	RrequestID = ord(data[1])
	print "TML: " + str(RTML)
	print "RrequestID: " + str(RrequestID)

	print "Returned answer is: " + str(data[2:RTML])
	print "len of data is :" + str(len(data))
	print "len of dataArray is :" + str(len(dataArray))
	
	requestID = requestID + 1;
	
else: 
	print "Please choose only from number 5, 80,or 10"
	
print "Elasped time: " + str(time.time() - startTime)

s.close()

print "received data:", data