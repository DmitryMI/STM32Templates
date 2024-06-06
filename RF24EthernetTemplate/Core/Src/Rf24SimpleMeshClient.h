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
#include "mbedtls.h"
#include "RF24Udp.h"

class Rf24SimpleMeshClient
{
public:
	Rf24SimpleMeshClient(SPI_HandleTypeDef *rf24_spi_handle);
	virtual ~Rf24SimpleMeshClient();

	void taskMethod();

private:
	RF24_SPI spi;
	EthernetClient rf24Client;
	RF24UDP rf24UdpClient;

	const IPAddress hostAddress = IPAddress(192, 168, 0, 2);
	const char* hostName = "192.168.0.2";
	const uint16_t hostPort = 1234;

	uint32_t counter = 0;
	uint32_t reqTimer = 0;
	uint32_t meshTimer = 0;

	int currentHostIndex = 0;

	// EOLs are required! Parsing fails without them.
	static constexpr const char *CertificateAuthority = "-----BEGIN CERTIFICATE-----\n"
			"MIIBmDCCAT0CFBe4QfkSzhG/qJOs+E4WIuxp244VMAoGCCqGSM49BAMCME4xCzAJ\n"
			"BgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRQwEgYDVQQHDAtMb3MgQW5n\n"
			"ZWxlczEUMBIGA1UECgwLVExTIFRlc3RpbmcwHhcNMjQwNjA2MTEyMzI1WhcNMzQw\n"
			"NjA0MTEyMzI1WjBOMQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEU\n"
			"MBIGA1UEBwwLTG9zIEFuZ2VsZXMxFDASBgNVBAoMC1RMUyBUZXN0aW5nMFkwEwYH\n"
			"KoZIzj0CAQYIKoZIzj0DAQcDQgAENiJtzyzHACo/3yI6E35/rwkN9NnzKssIxq9/\n"
			"mW2pLsBGm1utR1JXYvmRTDDeTK6b9keY53XEFqPsXOCPWR0c/zAKBggqhkjOPQQD\n"
			"AgNJADBGAiEA1uetfd5eNLodPviYtkZgoTsueLrXlL2OhpCQzgJvepwCIQCE/mh5\n"
			"rz9NVHZJ8kboBtS40o/2VFEekCoG8RtpN+cffg==\n"
			"-----END CERTIFICATE-----\n";

	bool setupRf24();
	void updateMesh();

	bool connectWithTls();

	static int entropySourceCallback(void *data, unsigned char *output, size_t len, size_t *olen);

	static int mbedtlsTcpSendCallback(void *ctx, const unsigned char *buf, size_t len);
	static int mbedtlsTcpRecvCallback(void *ctx, unsigned char *buf, size_t len);

	static int mbedtlsUdpSendCallback(void *ctx, const unsigned char *buf, size_t len);
	static int mbedtlsUdpRecvCallback(void *ctx, unsigned char *buf, size_t len);

	static void mbedtlsDebugPrintCallback(void *ctx, int level, const char *file, int line, const char *str);
	static void* mbedtlsCallockCallback(size_t num, size_t size);

	int mbedtlsTcpSendImpl(const unsigned char *buf, size_t len);
	int mbedtlsTcpRecvImpl(unsigned char *buf, size_t len);
	int mbedtlsUdpSendImpl(const unsigned char *buf, size_t len);
	int mbedtlsUdpRecvImpl(unsigned char *buf, size_t len);
};

#endif /* INC_RF24SIMPLEMESHCLIENT_H_ */
