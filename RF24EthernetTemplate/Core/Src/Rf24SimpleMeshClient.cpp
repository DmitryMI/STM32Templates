/*
 * Rf24SimpleMeshClient.cpp
 *
 *  Created on: May 24, 2024
 *      Author: DmitriyPC
 */

#include "Rf24SimpleMeshClient.h"

Rf24SimpleMeshClient::Rf24SimpleMeshClient(
		SPI_HandleTypeDef *rf24_spi_handle,
		GPIO_TypeDef *rf24_ce_port,
		uint16_t rf24_ce,
		GPIO_TypeDef *rf24_csn_port,
		uint16_t rf24_csn
	) :
		radio(encode_pin(rf24_ce_port, rf24_ce), encode_pin(rf24_csn_port, rf24_csn)),
		network(radio),
		mesh(radio, network),
		RF24Ethernet(
		radio, network, mesh
	)
{
	spi.begin(rf24_spi_handle);
}

Rf24SimpleMeshClient::~Rf24SimpleMeshClient()
{
	// TODO Auto-generated destructor stub
}

bool Rf24SimpleMeshClient::setup()
{
	if (!radio.begin(&spi))
	{
		printf("radio.begin() failed");
		return false;
	}

	IPAddress myIP(10, 10, 2, 4);
	Ethernet.begin(myIP);
	if (!mesh.begin())
	{
		printf("mesh.begin() failed");
		return false;
	}

	// If you'll be making outgoing connections from the Arduino to the rest of
	// the world, you'll need a gateway set up.
	IPAddress gwIP(10, 10, 2, 2);
	Ethernet.set_gateway(gwIP);

	printf("Rf24SimpleMeshClient::setup() OK");
	return true;
}

void Rf24SimpleMeshClient::update()
{
	if (HAL_GetTick() - meshTimer > 12000)
	{  //Every 12 seconds, test mesh connectivity
		meshTimer = HAL_GetTick();
		if (!mesh.checkConnection())
		{
			//refresh the network address
			if (mesh.renewAddress() == MESH_DEFAULT_ADDRESS)
			{
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
		while (HAL_GetTick() - reqTimer < 5000 && !client.available())
		{
		}
		connect();
	}
}

void Rf24SimpleMeshClient::connect()
{
	printf("connecting\n");

	if (client.connect(host, 80))
	{
		printf("connected\n");

		// Make an HTTP request:
		if (host == ascii)
		{
			client.println("GET http://artscene.textfiles.com/asciiart/texthistory.txt HTTP/1.1");
			client.println("Host: 208.86.224.90");
		}
		else
		{
			client.println(
					"GET /web/blyad.club/library/litrature/Salvatore,%20R.A/Salvatore,%20R.A%20-%20Icewind%20Dale%20Trilogy%201%20-%20Crystal%20Shard,%20The.txt HTTP/1.1");
			client.println("Host: 109.120.203.163");
		}

		client.println("Connection: close");
		client.println();
	}
	else
	{
		// if you didn't get a connection to the server:
		printf("connection failed\n");
	}
}
