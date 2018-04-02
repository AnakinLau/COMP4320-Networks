import socket
import sys
import struct
import binascii
import time

# Here argument 1 is the Server name(ie. tux057.eng.auburn.edu)
# And argument 2 is the port number (ie. 10031)
# Argument 3 is the request ID (a number from 0-127)
# Argument 4 to argument n is a hostname (ie. www.google.com)

if len(sys.argv) < 5:
	print "you must input argument hostname, followed by the port#, followed by a requestID number (0-127), then one more more strings of websites like www.google.com !"
	sys.exit()
	
#Compute the checksum of an item
def compute_checksum(buf):
    chksum = 0
    pointer = 0
    for element in buf:
        chksum += int(binascii.hexlify(element), 16)
        chksum = (chksum >> 8) + (chksum & 0xff)
    chksum += (chksum >> 8)
    return (~chksum) & 0xff
	

BUFFER_SIZE = 256
UDP_IP = socket.gethostbyname(sys.argv[1])
UDP_PORT = int(sys.argv[2]) + 10010 # Our GID + 10010
requestID = int(sys.argv[3])
hostNameArg = (sys.argv[4:]) # joins the string arguments with spaces???

startTime = time.time() # The start time

sock = socket.socket(socket.AF_INET, # Internet 
	socket.SOCK_DGRAM) # UDP

# Message Format:
# |Magic# 0x4A6F7921| TML | GID | Checksum | RequestID | Length1 | Hostname 1 | ... | Ln | Hn |
# | 	4 bytes		|  2  |  1  |     1    |      1    |    1    |  variable  | ... | 1  |var |


magicNum = 0x4A6F7921
GID = 21 # This is our group number 21
checksum = 00000000 # this is temporary 
TML = 0 # This is a 2byte value.

# We don't know the TML, so we just do the parts we know first.
packet = struct.pack("!BBB", GID, checksum, requestID)
TML = 3 # At this point

print "packet: " 
print struct.unpack_from("!BBB", packet)


# TML can have a total value of 65535??? Is this the buffer we should use?
testPackHostStr = "" # This is used to form the format for the debug message.

for x in hostNameArg: 
	packet = packet + struct.pack("!B",len(x))
	TML = TML + 1 # Add 1 byte for the length value
	packet = packet + struct.pack("!" + str(len(x)) + "s", x)
	TML = TML + len(x) # Add the length of our string var
	
	testPackHostStr = testPackHostStr + "B" + str(len(x)) + "s"
	
# Now we finally at the magic number and TML(now that we have it) in the front.
TML = TML + 4 + 2 # But not without adding 4 bytes for the magicNum and 2 for the TML First


packet = struct.pack("!IH", magicNum, TML) + packet

# Now we calculate the checksum
checksum = int(compute_checksum(packet))
print "checksum: " + str(checksum)

packedChecksum = struct.pack("!B", checksum)

packet = packet[:7] + packedChecksum + packet[8:]


print "packet = "


###print "# |Magic# 0x4A6F7921| TML | GID | Checksum | RequestID | Length1 | Hostname 1 | ... | Ln | Hn | "
###print "# | 	4 bytes		|  2  |  1  |     1    |      1    |    1    |  variable  | ... | 1  |var |"

print struct.unpack_from("!IHBBB" + testPackHostStr, packet) 

for x in hostNameArg:
	print x

validRes = False # Boolean for validity of response.
badrespFailCount = 0;
# redo?
while not(validRes):

	sock.sendto(packet, (UDP_IP, UDP_PORT)) # Sends the packet
	
	data = sock.recvfrom(1024) # Putting 1024 here for now... 

	realData = data[0]

	
	reHeader = struct.unpack_from("!IHBBB", realData, 0)
	#reHeader = struct.unpack_from("!IHBBB", data, 0)
	print "reHeader: " + str(reHeader)
	
	reTML = reHeader[1]
	reChecksum = reHeader[3]
	reMagicNum = reHeader[0]
	print "reTML: " + str(reTML)
	
	iter = 9 # Starting pos for IP addrs
	
	IPCount = (reTML - 9)/4 # Get the count of IPs returned.
	
	# Check length and TML validity, see if data is too short.
	if (((reTML > len(realData)) or (9 > reTML))):
		print "failed TML check"
		validRes = False
		
	# Check checksum integrity.
	# Somehow the index is somehow one off depending on packet...
	caldReChecksum = compute_checksum(realData[:reTML])
	caldReChecksum2 = compute_checksum(realData)
	if (caldReChecksum != 0 and caldReChecksum2 != 0):
		print "failed checksum check"
		validRes = False
		
		
	# Check Magic Number integrity.
	if not(int(reMagicNum) == 1248819489):
		print "failed Magic Num check"
		validRes = False
	
	# The above combined
	if ((((reTML <= len(realData)) and (9 <= reTML))) and ((caldReChecksum == 0) or (caldReChecksum2 == 0)) and (int(reMagicNum) == 1248819489)):
		validRes = True
		
	badrespFailCount = badrespFailCount + 1
	if not (validRes):
		print "Recieved invalid response from server, trying again now."
	
	if(badrespFailCount >= 7):
		print "Failed to get proper response from server more than 7 times, quitting now."
		exit()


# When there's no error.
if not(IPCount < 1):
	FourS_Str = ""
	# Try and form the "4s"'s required to unpack all our IPs together
	for num in range(0,IPCount): # This might be a very stupid way to do this, but can't think of much else ATM, will change if can later. :/
		FourS_Str = FourS_Str + "4s"
		
	IPs = struct.unpack_from("!" + FourS_Str, realData, 9)
	print "Requested Hosts: " + str(hostNameArg)
	print "Requested IPs: " + str(struct.unpack_from("!" + FourS_Str, realData, 9))


	print "Above are the IPs requested, but in mix of Hex, ASCII and Oct"

else: # There's an error in the request. 
	print "There was an error in the request."
	print "Byte Error Code from server: "
	print str(bin(reHeader[4]))


print "Elasped time: " + str(time.time() - startTime)