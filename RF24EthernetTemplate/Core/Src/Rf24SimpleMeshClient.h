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
			"MIIBmDCCAT0CFAWJfNLhwOuQARSbn8a3DawmI8h4MAoGCCqGSM49BAMCME4xCzAJ\n"
			"BgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRQwEgYDVQQHDAtMb3MgQW5n\n"
			"ZWxlczEUMBIGA1UECgwLVExTIFRlc3RpbmcwHhcNMjQwNjA2MTAwMjI5WhcNMzQw\n"
			"NjA0MTAwMjI5WjBOMQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEU\n"
			"MBIGA1UEBwwLTG9zIEFuZ2VsZXMxFDASBgNVBAoMC1RMUyBUZXN0aW5nMFkwEwYH\n"
			"KoZIzj0CAQYIKoZIzj0DAQcDQgAETooT37oYti/vqd+A3L6dJrpAqY30oEknyofG\n"
			"dPRK6yo3oakO+sFPoUgsdFlnXXKhH0H+FPloiUF6dxJHBBHRATAKBggqhkjOPQQD\n"
			"AgNJADBGAiEAj377/RfgBQOVy2mcdnvkszvY0EbxGohFqVJB9FjLaRQCIQCj2HtG\n"
			"y1yAxS3zAtzTODvXTX8QZGSzw4rjANZm9qE48Q==\n"
			"-----END CERTIFICATE-----";

	bool setupRf24();
	void updateMesh();

	static int entropySourceCallback(void *data, unsigned char *output, size_t len, size_t *olen);
	static int mbedtlsNetSendCallback( void *ctx, const unsigned char *buf, size_t len );
	static int mbedtlsNetRecvCallback( void *ctx, unsigned char *buf, size_t len );
	static int mbedtlsNetRecvTimeoutCallback( void *ctx, unsigned char *buf, size_t len,
            uint32_t timeout );
	static void mbedtlsDebugPrintCallback(void *ctx, int level, const char *file, int line,
            const char *str);
	static void* mbedtlsCallockCallback( size_t num, size_t size);
	int mbedtlsNetSendImpl(const unsigned char *buf, size_t len );
	int mbedtlsNetRecvImpl(unsigned char *buf, size_t len );
	int mbedtlsNetRecvTimeoutImpl(unsigned char *buf, size_t len,
	                      uint32_t timeout);
};

#endif /* INC_RF24SIMPLEMESHCLIENT_H_ */
