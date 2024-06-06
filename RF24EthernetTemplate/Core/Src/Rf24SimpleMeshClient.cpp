/*
 * Rf24SimpleMeshClient.cpp
 *
 *  Created on: May 24, 2024
 *      Author: DmitriyPC
 */

#include "Rf24SimpleMeshClient.h"
#include "cmsis_os.h"
#include "mbedtls.h"
#include "net_sockets.h"
#include "platform.h"

RF24 radio(encode_pin(RF24_CE_GPIO_Port, RF24_CE_Pin), encode_pin(RF24_CSN_GPIO_Port, RF24_CSN_Pin));
RF24Network network(radio);
RF24Mesh mesh(radio, network);

// Needs to be defined as a global variable, because the RF24Ethernet library declared this as external variable.
RF24EthernetClass RF24Ethernet(radio, network, mesh);

Rf24SimpleMeshClient::Rf24SimpleMeshClient(SPI_HandleTypeDef *rf24_spi_handle)
{
	spi.begin(rf24_spi_handle);
}

Rf24SimpleMeshClient::~Rf24SimpleMeshClient()
{
	// TODO Auto-generated destructor stub
}

int Rf24SimpleMeshClient::entropySourceCallback(void *data, unsigned char *output, size_t len, size_t *olen)
{
#ifndef HAL_RNG_MODULE_ENABLED
#warning "TRNG support is not enabled or is not available! TLS requires TRNG to be secure! No-TRNG mode should only be used for testing!"
	uint32_t rnd = rand();
	size_t num = sizeof(uint32_t);
	if (len < num)
	{
		num = len;
	}
	memcpy(output, (uint8_t*) &rnd, num);
	*olen = num;
	return 0;

#endif
}

void* Rf24SimpleMeshClient::mbedtlsCallockCallback(size_t num, size_t size)
{
	void *mem = pvPortMalloc(num * size);
	if (mem == nullptr)
	{
		return nullptr;
	}

	memset(mem, 0, num * size);
	return mem;
}

int Rf24SimpleMeshClient::mbedtlsTcpSendCallback(void *ctx, const unsigned char *buf, size_t len)
{
	Rf24SimpleMeshClient *client = static_cast<Rf24SimpleMeshClient*>(ctx);
	return client->mbedtlsTcpSendImpl(buf, len);
}

int Rf24SimpleMeshClient::mbedtlsTcpRecvCallback(void *ctx, unsigned char *buf, size_t len)
{
	Rf24SimpleMeshClient *client = static_cast<Rf24SimpleMeshClient*>(ctx);
	return client->mbedtlsTcpRecvImpl(buf, len);
}

int Rf24SimpleMeshClient::mbedtlsUdpSendCallback(void *ctx, const unsigned char *buf, size_t len)
{
	Rf24SimpleMeshClient *client = static_cast<Rf24SimpleMeshClient*>(ctx);
	return client->mbedtlsUdpSendImpl(buf, len);
}

int Rf24SimpleMeshClient::mbedtlsUdpRecvCallback(void *ctx, unsigned char *buf, size_t len)
{
	Rf24SimpleMeshClient *client = static_cast<Rf24SimpleMeshClient*>(ctx);
	return client->mbedtlsUdpRecvImpl(buf, len);
}

void Rf24SimpleMeshClient::mbedtlsDebugPrintCallback(void *ctx, int level, const char *file, int line, const char *str)
{
	const char *p, *basename;
	(void) ctx;

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++)
	{
		if (*p == '/' || *p == '\\')
		{
			basename = p + 1;
		}
	}

	printf("[Rf24SimpleMeshClient] %s:%04d: |%d| %s", basename, line, level, str);
}

int Rf24SimpleMeshClient::mbedtlsTcpSendImpl(const unsigned char *buf, size_t len)
{
	int res = rf24Client.write(buf, len);
	updateMesh();
	RF24Ethernet.update();
	return res;
}

int Rf24SimpleMeshClient::mbedtlsTcpRecvImpl(unsigned char *buf, size_t len)
{
	uint32_t timeout = HAL_GetTick() + 60000;
	size_t leftToRead = len;
	while (leftToRead > 0 && HAL_GetTick() < timeout)
	{
		updateMesh();
		size_t available = (size_t) rf24Client.available();
		if (available == 0)
		{
			// osThreadYield();
			continue;
		}

		int read;
		if (available < leftToRead)
		{
			read = rf24Client.read(buf, available);
		}
		else
		{
			read = rf24Client.read(buf, leftToRead);
		}

		leftToRead -= read;
	}

	return len - leftToRead;
}

int Rf24SimpleMeshClient::mbedtlsUdpSendImpl(const unsigned char *buf, size_t len)
{
	if (!rf24UdpClient.beginPacket(hostAddress, hostPort))
	{
		return -1;
	}

	rf24UdpClient.write(buf, len);

	if (!rf24UdpClient.endPacket())
	{
		return -1;
	}

	return len;
}

int Rf24SimpleMeshClient::mbedtlsUdpRecvImpl(unsigned char *buf, size_t len)
{
	int size = 0;
	while (true)
	{
		size = rf24UdpClient.parsePacket();
		if (size == 0)
		{
			osThreadYield();
			continue;
		}

		if (rf24UdpClient.remoteIP() != hostAddress || rf24UdpClient.remotePort() != hostPort)
		{
			rf24UdpClient.flush();
			osThreadYield();
			continue;
		}

		int read = rf24UdpClient.read(buf, len);
		rf24UdpClient.flush();
		return read;
	}
}

void Rf24SimpleMeshClient::taskMethod()
{
	while (true)
	{
		if (connectWithTls())
		{
			break;
		}

		printf("FAILURE\n\n");
		osDelay(3000);
	}
}

bool Rf24SimpleMeshClient::connectWithTls()
{
	bool success = false;

	if (!setupRf24())
	{
		return false;
	}

	rf24UdpClient.begin(1234);

	uint32_t stackHighWaterMark;

	mbedtls_platform_set_calloc_free(&Rf24SimpleMeshClient::mbedtlsCallockCallback, vPortFree);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// TRUST CHAIN CONFIGURATION

	mbedtls_x509_crt x509_certificate;
	mbedtls_x509_crt_init(&x509_certificate);

	int mbedtls_status;

	if ((mbedtls_status = mbedtls_x509_crt_parse(&x509_certificate, (const unsigned char*) CertificateAuthority, strlen(CertificateAuthority) + 1)) != 0)
	{
		printf("[Rf24SimpleMeshClient] [!] mbedtls_x509_crt_parse_file failed to parse CA certificate (-0x%X)\n", -mbedtls_status);
		goto quit_x509_certificate;
	}

	printf("[Rf24SimpleMeshClient] Certificate Authority parsing OK.\n");
	stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	printf("[Rf24SimpleMeshClient] Stack high water mark: %lu\n", stackHighWaterMark);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ENTROPY/RANDOMNESS SOURCE AND PSEUDORANDOM NUMBER GENERATOR (PRNG) CONFIGURATION

	mbedtls_entropy_context entropy_context;
	mbedtls_entropy_init(&entropy_context);

	mbedtls_ctr_drbg_context drbg_context;
	mbedtls_ctr_drbg_init(&drbg_context);

	mbedtls_entropy_add_source(&entropy_context, &Rf24SimpleMeshClient::entropySourceCallback, nullptr, 0, MBEDTLS_ENTROPY_SOURCE_STRONG);

	if ((mbedtls_status = mbedtls_ctr_drbg_seed(&drbg_context, mbedtls_entropy_func, &entropy_context, nullptr, 0)) != 0)
	{
		printf("[Rf24SimpleMeshClient] [!] mbedtls_ctr_drbg_seed (-0x%X)\n", -mbedtls_status);
		goto quit_entropy;
	}

	printf("[Rf24SimpleMeshClient] Entropy source OK\n");
	stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	printf("[Rf24SimpleMeshClient] Stack high water mark: %lu\n", stackHighWaterMark);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// TLS CONFIGURATION

	mbedtls_ssl_config ssl_config;
	mbedtls_ssl_config_init(&ssl_config);
	mbedtls_ssl_conf_dbg(&ssl_config, &Rf24SimpleMeshClient::mbedtlsDebugPrintCallback, this);
	mbedtls_debug_set_threshold(5);

	if ((mbedtls_status = mbedtls_ssl_config_defaults(&ssl_config,
	MBEDTLS_SSL_IS_CLIENT,
	MBEDTLS_SSL_TRANSPORT_DATAGRAM,
	MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
		printf("[Rf24SimpleMeshClient] [!] mbedtls_ssl_config_defaults failed to load default SSL config (-0x%X)\n", -mbedtls_status);
		goto quit_ssl_config;
	}

	printf("[Rf24SimpleMeshClient] mbedtls_ssl_config_defaults OK\n");
	stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	printf("[Rf24SimpleMeshClient] Stack high water mark: %lu\n", stackHighWaterMark);

	// Only use this cipher suite
	// static const int tls_cipher_suites[2] = { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, 0 };
	// mbedtls_ssl_conf_ciphersuites(&ssl_config, tls_cipher_suites);

	// By limiting ourselves to TLS v1.2 and the previous cipher suites, we can compile mbedTLS without the unused ciphers
	// and reduce its size

	// Load CA certificate
	mbedtls_ssl_conf_ca_chain(&ssl_config, &x509_certificate, nullptr);
	// Strictly ensure that certificates are signed by the CA
	mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random, &drbg_context);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// TLS CONTEXT

	mbedtls_ssl_context ssl_context;
	mbedtls_ssl_init(&ssl_context);

	if ((mbedtls_status = mbedtls_ssl_setup(&ssl_context, &ssl_config)) != 0)
	{
		printf("[Rf24SimpleMeshClient] [!] mbedtls_ssl_setup failed to setup SSL context (-0x%X)\n", -mbedtls_status);
		goto quit_ssl_context;
	}

	printf("[Rf24SimpleMeshClient] TLS context OK\n");
	stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	printf("[Rf24SimpleMeshClient] Stack high water mark: %lu\n", stackHighWaterMark);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ESTABLISH SECURE TLS CONNECTION

	printf("[Rf24SimpleMeshClient] RF24 TCP/IP connection OK\n");
	stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	printf("[Rf24SimpleMeshClient] Stack high water mark: %lu\n", stackHighWaterMark);

	mbedtls_ssl_conf_max_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
	mbedtls_ssl_conf_min_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

	mbedtls_ssl_set_bio(&ssl_context, this, &Rf24SimpleMeshClient::mbedtlsUdpSendCallback, &Rf24SimpleMeshClient::mbedtlsUdpRecvCallback, nullptr);

	// Verify that that certificate actually belongs to the host
	if ((mbedtls_status = mbedtls_ssl_set_hostname(&ssl_context, hostName)) != 0)
	{
		printf("[Rf24SimpleMeshClient] [!] mbedtls_ssl_set_hostname (-0x%X)\n", -mbedtls_status);
		goto quit_close_context;
	}

	printf("[Rf24SimpleMeshClient] mbedtls_ssl_set_hostname OK\n");
	stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	printf("[Rf24SimpleMeshClient] Stack high water mark: %lu\n", stackHighWaterMark);

	while ((mbedtls_status = mbedtls_ssl_handshake(&ssl_context)) != 0)
	{
		if (mbedtls_status != MBEDTLS_ERR_SSL_WANT_READ && mbedtls_status != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			printf("[Rf24SimpleMeshClient] [!] mbedtls_ssl_handshake (-0x%X)\n", -mbedtls_status);
			goto quit_close_context;
		}
		// osThreadYield();
	}

	printf("[Rf24SimpleMeshClient] mbedtls_ssl_handshake OK\n");

	if ((mbedtls_status = mbedtls_ssl_get_verify_result(&ssl_context)) != 0)
	{
		printf("[Rf24SimpleMeshClient] [!] mbedtls_ssl_get_verify_result (-0x%X)\n", -mbedtls_status);
		goto quit_close_context;
	}
	stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	printf("[Rf24SimpleMeshClient] Stack high water mark: %lu\n", stackHighWaterMark);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// EXCHANGE SOME MESSAGES

	static const unsigned char write_buffer[] = "Hello world!\n";
	static const auto write_buffer_length = sizeof(write_buffer) - 1; // last byte is the null terminator

	do
	{
		mbedtls_status = mbedtls_ssl_write(&ssl_context, write_buffer + mbedtls_status, write_buffer_length - mbedtls_status);

		if (mbedtls_status == 0)
		{
			break;
		}

		if (mbedtls_status < 0)
		{
			switch (mbedtls_status)
			{
			case MBEDTLS_ERR_SSL_WANT_READ:
			case MBEDTLS_ERR_SSL_WANT_WRITE:
			case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
			case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
			{
				continue;
			}
			default:
			{
				printf("[Rf24SimpleMeshClient] [!] mbedtls_ssl_write (-0x%X)\n", -mbedtls_status);
				goto quit_close_context;
			}
			}
		}

		printf("[Rf24SimpleMeshClient] [*] %d bytes sent to the server\n", mbedtls_status);
	}
	while (true);

	do
	{
		unsigned char read_buffer[64];
		static const auto read_buffer_length = sizeof(read_buffer);

		memset(read_buffer, 0, sizeof(read_buffer));

		mbedtls_status = mbedtls_ssl_read(&ssl_context, read_buffer, read_buffer_length);

		if (mbedtls_status == 0)
		{
			break;
		}

		if (mbedtls_status < 0)
		{
			switch (mbedtls_status)
			{
			case MBEDTLS_ERR_SSL_WANT_READ:
			case MBEDTLS_ERR_SSL_WANT_WRITE:
			case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
			case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
			{
				continue;
			}
			default:
			{
				printf("[Rf24SimpleMeshClient] [!] mbedtls_ssl_read (-0x%X)\n", -mbedtls_status);
				goto quit_close_context;
			}
			}
		}

		auto line_terminator_received = false;

		for (auto i = 0; i < mbedtls_status; ++i)
		{
			if (read_buffer[i] == '\n')
			{
				line_terminator_received = true;
				break;
			}
		}

		if (line_terminator_received)
		{
			if (mbedtls_status > 1)
			{
				printf("[Rf24SimpleMeshClient] [*] Received chunk '%.*s'\n", mbedtls_status - 1, reinterpret_cast<char*>(read_buffer));
			}
			break;
		}

		printf("[Rf24SimpleMeshClient] [*] Received chunk '%.*s'\n", mbedtls_status, reinterpret_cast<char*>(read_buffer));
	}
	while (true);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	success = true;

quit_close_context:

	// In our protocol, the connection will be closed by the server first
#if 0
		if ((status = mbedtls_ssl_close_notify(&ssl_context)) != 0)
		{
			  printf("[Rf24SimpleMeshClient] [!] mbedtls_ssl_close_notify (-0x%X)\n", -status);
		}
	#endif

quit_ssl_context:
	mbedtls_ssl_free(&ssl_context);

quit_ssl_config:
	mbedtls_ssl_config_free(&ssl_config);

quit_entropy:
	mbedtls_ctr_drbg_free(&drbg_context);
	mbedtls_entropy_free(&entropy_context);

quit_x509_certificate:
	mbedtls_x509_crt_free(&x509_certificate);

	rf24UdpClient.stop();
	return success;
}

bool Rf24SimpleMeshClient::setupRf24()
{

	if (!radio.begin(&spi))
	{
		printf("[Rf24SimpleMeshClient] radio.begin() failed\n");
		return false;
	}

	printf("[Rf24SimpleMeshClient] radio.begin() OK\n");

	if (!radio.isPVariant())
	{
		printf("[Rf24SimpleMeshClient] [!] RF24 is not P-Variant!\n");
	}

	printf("[Rf24SimpleMeshClient] RF24 is P-Variant.\n");

	if (!radio.setDataRate(RF24_1MBPS))
	{
		printf("[Rf24SimpleMeshClient] radio.setDataRate() failed\n");
		return false;
	}

	printf("[Rf24SimpleMeshClient] radio.setDataRate() OK\n");

	IPAddress myIP(10, 10, 2, 32);
	Ethernet.begin(myIP);

	const int retryMillis = 3000;
	while (!mesh.begin())
	{
		printf("[Rf24SimpleMeshClient] mesh.begin() failed. Retrying...\n");

		// HardFaults if task stack size is 128 words. 256 words work fine.
		osDelay(retryMillis);
	}

	printf("[Rf24SimpleMeshClient] mesh.begin() OK\n");

// If you'll be making outgoing connections from the Arduino to the rest of
// the world, you'll need a gateway set up.
	IPAddress gwIP(10, 10, 2, 0);
	Ethernet.set_gateway(gwIP);

	printf("[Rf24SimpleMeshClient] RF24 setup OK\n");
	return true;
}

void Rf24SimpleMeshClient::updateMesh()
{
	if (HAL_GetTick() - meshTimer > 12000)
	{  //Every 12 seconds, test mesh connectivity
		meshTimer = HAL_GetTick();
		if (!mesh.checkConnection())
		{
			printf("Trying to renew mesh address...\n");
			//refresh the network address
			if (mesh.renewAddress() == MESH_DEFAULT_ADDRESS)
			{
				printf("Mesh address renewed\n");
				mesh.begin();
			}
		}
	}
}
