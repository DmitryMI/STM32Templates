openssl genrsa -out ca-prk.pem 2048

openssl req -new -sha256 -key ca-prk.pem -out ca-csr.pem -subj "/C=US/ST=California/L=Los Angeles/O=TLS Testing"

openssl x509 -req -signkey ca-prk.pem -in ca-csr.pem -out ca-cer.pem -days 3650


openssl genrsa -out server-prk.pem 2048

openssl req -new -sha256 -key server-prk.pem -out server-csr.pem -subj "/C=US/ST=California/L=Los Angeles/O=TLS Testing/CN=127.0.0.1"

openssl req -in server-csr.pem -noout -text

openssl x509 -req -sha256 -in server-csr.pem -CA ca-cer.pem -CAkey ca-prk.pem -CAcreateserial -out server-cer.pem -days 365

openssl x509 -in server-cer.pem -noout -text
