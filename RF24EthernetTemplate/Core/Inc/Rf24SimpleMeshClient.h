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
#include "IPAddress.h"

class Rf24SimpleMeshClient
{
public:
	Rf24SimpleMeshClient(
			SPI_HandleTypeDef *rf24_spi_handle,
			GPIO_TypeDef *rf24_ce_port,
			uint16_t rf24_ce,
			GPIO_TypeDef *rf24_csn_port,
			uint16_t rf24_csn
			);
	virtual ~Rf24SimpleMeshClient();

	bool setup();

	void update();

private:
	RF24_SPI spi;
	RF24 radio;
	RF24Network network;
	RF24Mesh mesh;
	RF24EthernetClass RF24Ethernet;
	EthernetClient client;

	uint32_t counter = 0;
	uint32_t reqTimer = 0;
	uint32_t meshTimer = 0;

	IPAddress icewind = IPAddress(109, 120, 203, 163);  //http://109.120.203.163/web/blyad.club/library/litrature/Salvatore,%20R.A/Salvatore,%20R.A%20-%20Icewind%20Dale%20Trilogy%201%20-%20Crystal%20Shard,%20The.txt
	IPAddress ascii = IPAddress(208, 86, 224, 90);      //http://artscene.textfiles.com/asciiart/texthistory.txt
	IPAddress host = ascii;

	void connect();
};

#endif /* INC_RF24SIMPLEMESHCLIENT_H_ */
