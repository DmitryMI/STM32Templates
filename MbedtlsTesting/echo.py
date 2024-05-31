#!/usr/bin/python3
# usage python3 echoTcpServer.py [bind IP]  [bind PORT]

import socket
import sys
import string
import random

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# Bind the socket to the port
server_address = ((sys.argv[1]), (int(sys.argv[2])))
sock.bind(server_address)
# Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print('waiting for a connection')
    connection, client_address = sock.accept()
    try:
        print('connection from', client_address)

        # Receive the data in small chunks and retransmit it
        while True:
            data = random.choice(string.ascii_letters)
            connection.sendall(bytes(data, 'utf-8'))

    finally:
        # Clean up the connection
        connection.close()