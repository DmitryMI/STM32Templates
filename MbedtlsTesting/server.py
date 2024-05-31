#!/usr/bin/python
# -*- coding: utf-8 -*-
import logging
import sys
import socket
import ssl

SERVER_ADDRESS = ('0.0.0.0', 1234)

logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)

# Allow only TLS 1.2
tls_context = ssl.SSLContext(protocol=ssl.PROTOCOL_TLSv1_2)
# Allow only these cipher suites
# tls_context.set_ciphers('ECDHE-RSA-AES128-GCM-SHA256')
tls_context.set_ciphers('ECDHE-ECDSA-AES128-GCM-SHA256')
# Load server certificate and private key
tls_context.load_cert_chain('server-cer.pem', keyfile='server-prk.pem')


def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_host, server_port = SERVER_ADDRESS
    server_socket.bind((server_host, server_port))
    server_socket.listen(1)
    logging.info(f'Listening on {server_host}:{server_port}')
    while True:
        try:
            client_socket, client_address = server_socket.accept()
            with client_socket:
                with tls_context.wrap_socket(client_socket, server_side=True) as client_tls_socket:
                    tls_cipher, tls_version, _ = client_tls_socket.cipher()
                    client_host, client_port = client_address
                    logging.info(f'Accepted connection from {client_host}:{client_port} ({tls_version} {tls_cipher})')
                    while True:
                        message = client_tls_socket.recv(64)
                        if message and message[-1] == 10:
                            logging.info(f'Received chunk "{message[:-1].decode("utf-8")}"')
                            break
                        logging.info(f'Received chunk "{message.decode("utf-8")}"')
                    client_tls_socket.sendall('Bye world!\n'.encode('utf-8'))
                    logging.info(f'Data sent to the client')
        except Exception as err:
            logging.error(err)


if __name__ == '__main__':
    main()