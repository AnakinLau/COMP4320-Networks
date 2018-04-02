import socket
import sys
import struct
import binascii

if len(sys.argv) < 2:
	print "You must input argument port#!"
	sys.exit()
	
myGID = 21	
BUFFER_SIZE = 256
UDP_IP = socket.gethostbyname(socket.gethostname())
UDP_PORT = int(sys.argv[1]) # Our GID * 5 + 10010 = 10115


sock = socket.socket(socket.AF_INET, # Internet 
	socket.SOCK_DGRAM) # UDP
	
sock.bind((UDP_IP, UDP_PORT))

# Variables for the waiting client info
waitingIP = ""
waitingPort = 0

errorCode = 0 # Will be manipulated to be send back to client.
isWaiting = False;


while True:
	data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
	print "received message:", data
	# Joy!, Ph, Pl, GID(Client)
	unpackedData = struct.unpack_from("!4sHB", data)
	print "Length of data received: " + str(len(data))
	print "Raw data received as MagicNumber, Ph, Pl, GID"
	print struct.unpack_from("!4sBBB", data) 
	print "Raw data received as MagicNumber, Port, GID"
	print struct.unpack_from("!4sHB", data) 
	print "Address displayed in reversed Network Order."
	print str(addr[0])
	print "Below is the port received in reversed Network Order."
	print struct.unpack_from("!H", str(addr[1]))
	# Do some error checking. Send back 0111 or 0100 or 0101 or 0011 or whatevers
	errorCode = 0
	if (unpackedData[0] != "Joy!"):
		# Magic Number is wrong
		errorCode = 1
	
	if (len(data) != 7):
		# Length is wrong
		errorCode = errorCode + 2
	
	if ((unpackedData[1] > (10010 + (unpackedData[2] * 5) + 4)) or (unpackedData[1] < (10010 + (unpackedData[2] * 5)))):
		# Port is out of range for client
		errorCode = errorCode + 4
	
	print "errorCode " + str(errorCode)
	if (errorCode > 0):
		print "There was an error with the request."
		# Error Packet Format: MagicNumber GID 00 XY(ErrorCode)
		# WTH is the GID to use here? The server's or the client's?
		errorPacket = struct.pack("!4sBBB", "Joy!", unpackedData[2], 0, errorCode)
		sock.sendto(errorPacket, addr)
	elif(not isWaiting): # The request is valid. If there's no one waiting.
		print "Returning info for the client to wait."
		waitInstrPacket = struct.pack("!4sBH", "Joy!", myGID, unpackedData[1])
		sock.sendto(waitInstrPacket, addr)
		waitingIP = addr[0]
		waitingPort = unpackedData[1]
		isWaiting = True
	else: # Below someone is waiting.
		print "Someone is waiting, will send you the info now!"
		chatInfoPacket = struct.pack("!4s", "Joy!")
		chatInfoPacket = chatInfoPacket + socket.inet_aton(waitingIP) 
		chatInfoPacket = chatInfoPacket + struct.pack("!HB", waitingPort, myGID)
		sock.sendto(chatInfoPacket, addr)
		waitingIP = "" # Clear the infos.
		waitingPort = 0
		isWaiting = False
		break

