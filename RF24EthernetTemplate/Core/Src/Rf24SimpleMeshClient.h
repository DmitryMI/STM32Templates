/*
 * Rf24SimpleMeshClient.h
 *
 *  Created on: May 24, 2024
 *      Author: DmitriyPC
 */

#ifndef INC_RF24SIMPLEMESHCLIENT_H_
#define INC_RF24SIMPLEMESHCLIENT_H_

#include "main.h"
#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <RF24Ethernet.h>
#include <array>
#include "IPAddress.h"

class Rf24SimpleMeshClient
{
public:
	Rf24SimpleMeshClient(SPI_HandleTypeDef *rf24_spi_handle);
	virtual ~Rf24SimpleMeshClient();

	void taskMethod();

private:
	RF24_SPI spi;
	EthernetClient rf24Client;

	uint32_t counter = 0;
	uint32_t reqTimer = 0;
	uint32_t meshTimer = 0;

	int currentHostIndex = 0;

	// EOLs are required! Parsing fails without them.
	static constexpr const char *CertificateAuthority =
			"-----BEGIN CERTIFICATE-----\n"
			"MIIBlzCCAT0CFAgW+gEfvYrwSYjElMJLxDMU/0XvMAoGCCqGSM49BAMCME4xCzAJ\n"
			"BgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRQwEgYDVQQHDAtMb3MgQW5n\n"
			"ZWxlczEUMBIGA1UECgwLVExTIFRlc3RpbmcwHhcNMjQwNTI2MTMyMTE4WhcNMzQw\n"
			"NTI0MTMyMTE4WjBOMQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEU\n"
			"MBIGA1UEBwwLTG9zIEFuZ2VsZXMxFDASBgNVBAoMC1RMUyBUZXN0aW5nMFkwEwYH\n"
			"KoZIzj0CAQYIKoZIzj0DAQcDQgAETitrDALqUZGTCYCjikqjSk4a2o8/aPAcocmu\n"
			"YF0OGQnQXYMpiQ289bWewzJ7LEEsToGfyA2SHW3qvCcmZkNh0TAKBggqhkjOPQQD\n"
			"AgNIADBFAiEAvOE+0ES4Tyu7geZ/7rl/hDE9C9rj1qwWZUpPEWowbx8CIByrs6F4\n"
			"Q1jJj8xPOg9sgc55XI8v/ZAfnzFD4oQamqa/\n"
			"-----END CERTIFICATE-----";

	bool setupRf24();
	void updateMesh();

	static int entropySourceCallback(void *data, unsigned char *output, size_t len, size_t *olen);
	static int mbedtlsNetSendCallback( void *ctx, const unsigned char *buf, size_t len );
	static int mbedtlsNetRecvCallback( void *ctx, unsigned char *buf, size_t len );
	static int mbedtlsNetRecvTimeoutCallback( void *ctx, unsigned char *buf, size_t len,
            uint32_t timeout );
	int mbedtlsNetSendImpl(const unsigned char *buf, size_t len );
	int mbedtlsNetRecvImpl(unsigned char *buf, size_t len );
	int mbedtlsNetRecvTimeoutImpl(unsigned char *buf, size_t len,
	                      uint32_t timeout);
};

#endif /* INC_RF24SIMPLEMESHCLIENT_H_ */
