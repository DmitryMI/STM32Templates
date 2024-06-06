set -e

# Generate a new ECDSA private key
openssl ecparam -genkey -name secp112r1 -out ca-prk.pem

# Create a new certificate signing request (CSR) using the ECDSA private key
openssl req -new -sha1 -key ca-prk.pem -out ca-csr.pem -subj "/C=US/ST=California/L=Los Angeles/O=TLS Testing"

# Sign the CSR with the ECDSA private key to create the certificate
openssl x509 -req -signkey ca-prk.pem -in ca-csr.pem -out ca-cer.pem -days 3650
