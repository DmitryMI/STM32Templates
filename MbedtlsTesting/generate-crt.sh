set -e

# Generate a new ECDSA private key
openssl ecparam -genkey -name prime256v1 -out server-prk.pem

# Create a new certificate signing request (CSR) using the ECDSA private key
openssl req -new -sha256 -key server-prk.pem -out server-csr.pem -subj "/C=US/ST=0/L=0/O=0/CN=192.168.0.6"

# Display the CSR details
openssl req -in server-csr.pem -noout -text

# Sign the CSR with the CA's private key to create the server certificate
openssl x509 -req -sha256 -in server-csr.pem -CA ca-cer.pem -CAkey ca-prk.pem -CAcreateserial -out server-cer.pem -days 3650

# Display the server certificate details
openssl x509 -in server-cer.pem -noout -text

