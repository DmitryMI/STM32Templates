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
    conn, addr = sock.accept()
    with conn:
        while True:
            data = conn.recv(1024)
            print(data)
            conn, addr = sock.accept()