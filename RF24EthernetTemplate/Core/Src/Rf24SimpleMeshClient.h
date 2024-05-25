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
	EthernetClient client;

	uint32_t counter = 0;
	uint32_t reqTimer = 0;
	uint32_t meshTimer = 0;

	int currentHostIndex = 0;

	bool setup();
	void loop();
	void clientConnect();
};

#endif /* INC_RF24SIMPLEMESHCLIENT_H_ */
