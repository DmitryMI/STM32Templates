/*
 * Rf24SimpleMeshClient.cpp
 *
 *  Created on: May 24, 2024
 *      Author: DmitriyPC
 */

#include "Rf24SimpleMeshClient.h"
#include "cmsis_os.h"
#include "mbedtls.h"

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

void Rf24SimpleMeshClient::taskMethod()
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// TRUST CHAIN CONFIGURATION

	mbedtls_x509_crt x509_certificate;
	mbedtls_x509_crt_init(&x509_certificate);

	int mbedtls_status;

	if ((mbedtls_status = mbedtls_x509_crt_parse(&x509_certificate, (const unsigned char*) CertificateAuthority, strlen(CertificateAuthority) + 1)) != 0)
	{
		printf("[!] mbedtls_x509_crt_parse_file failed to parse CA certificate (-0x%X)\n", -mbedtls_status);
		return;
	}

	printf("Certificate Authority parsing OK.");

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ENTROPY/RANDOMNESS SOURCE AND PSEUDORANDOM NUMBER GENERATOR (PRNG) CONFIGURATION
	/*
	mbedtls_entropy_context entropy_context;
	mbedtls_entropy_init(&entropy_context);

	mbedtls_ctr_drbg_context drbg_context;
	mbedtls_ctr_drbg_init(&drbg_context);

	if ((mbedtls_status = mbedtls_ctr_drbg_seed(&drbg_context, mbedtls_entropy_func, &entropy_context, nullptr, 0)) != 0)
	{
		printf("[!] mbedtls_ctr_drbg_seed (-0x%X)\n", -mbedtls_status);
		// goto quite_entropy;
	}
	*/
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (!setup())
	{
		return;
	}

	while (true)
	{
		loop();
		osThreadYield();
	}
}

bool Rf24SimpleMeshClient::setup()
{
	printf("Rf24SimpleMeshClient started\n");

	if (!radio.begin(&spi))
	{
		printf("radio.begin() failed\n");
		return false;
	}
	else
	{
		printf("radio.begin() OK\n");
	}

	IPAddress myIP(10, 10, 2, 32);
	Ethernet.begin(myIP);

	const int retryMillis = 3000;
	while (!mesh.begin())
	{
		printf("mesh.begin() failed. Retrying...\n");

		// HardFaults if task stack size is 128 words. 256 words work fine.
		osDelay(retryMillis);
	}

	printf("mesh.begin() OK\n");

	// If you'll be making outgoing connections from the Arduino to the rest of
	// the world, you'll need a gateway set up.
	IPAddress gwIP(10, 10, 2, 0);
	Ethernet.set_gateway(gwIP);

	printf("Rf24SimpleMeshClient::setup() OK\n");
	return true;
}

void Rf24SimpleMeshClient::loop()
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

	size_t size;

	if ((size = client.available()) > 0)
	{
		char c = client.read();
		putchar(c);
		counter++;
	}

	// if the server's disconnected, stop the client:
	if (!client.connected())
	{
		printf("\n");
		printf("Disconnect. Waiting for disconnect timeout\n");
		client.stop();

		// Wait 5 seconds between requests
		// Calling client.available(); or Ethernet.update(); is required during delays
		// to keep the stack updated
		reqTimer = HAL_GetTick();
		while (HAL_GetTick() - reqTimer < 1000 && !client.available())
		{
			osThreadYield();
		}

		clientConnect();
	}
}

void Rf24SimpleMeshClient::clientConnect()
{
	IPAddress host1Ip = IPAddress(109, 120, 203, 163);// http://109.120.203.163/web/blyad.club/library/litrature/Salvatore,%20R.A/Salvatore,%20R.A%20-%20Icewind%20Dale%20Trilogy%201%20-%20Crystal%20Shard,%20The.txt
	const char *host1Get = "GET http://artscene.textfiles.com/asciiart/texthistory.txt HTTP/1.1";
	const char *host1Host = "Host: 208.86.224.90";

	IPAddress host2Ip = IPAddress(208, 86, 224, 90);	// http://artscene.textfiles.com/asciiart/texthistory.txt
	const char *host2Get =
			"GET /web/blyad.club/library/litrature/Salvatore,%20R.A/Salvatore,%20R.A%20-%20Icewind%20Dale%20Trilogy%201%20-%20Crystal%20Shard,%20The.txt HTTP/1.1";
	const char *host2Host = "Host: 109.120.203.163";

	IPAddress hostCurrentIp;
	const char *hostCurrentGet;
	const char *hostCurrentHost;

	switch (currentHostIndex)
	{
	case 0:
		hostCurrentIp = host1Ip;
		hostCurrentGet = host1Get;
		hostCurrentHost = host1Host;
		break;
	case 1:
		hostCurrentIp = host2Ip;
		hostCurrentGet = host2Get;
		hostCurrentHost = host2Host;
	}

	currentHostIndex++;
	if (currentHostIndex == 2)
	{
		currentHostIndex = 0;
	}

	printf("connecting to %d.%d.%d.%d\n", hostCurrentIp[0], hostCurrentIp[1], hostCurrentIp[2], hostCurrentIp[3]);

	if (client.connect(hostCurrentIp, 80))
	{
		printf("connected\n");

		client.println(hostCurrentGet);
		client.println(hostCurrentHost);

		client.println("Connection: close");
		client.println();
	}
	else
	{
		// if you didn't get a connection to the server:
		printf("connection failed\n");
	}
}
