#pragma once

#include <array>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef CM_UNIX
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <RF24Mesh/RF24Mesh.h>
#include <RF24Gateway/RF24Gateway.h>
#endif

class RF24GatewayHandler
{
public:
	/// <summary>
	/// Gateway constructor
	/// </summary>
	/// <param name="CePin">CE pin. Uses RPi GPIO numbering</param>
	/// <param name="CsnPin">SPI Bus number. 00 for /dev/spidev0.0, 11 for /dev/spidev1.1, etc</param>
	RF24GatewayHandler(uint16_t CePin, uint16_t CsnPin);
	~RF24GatewayHandler();

	bool Begin(const std::array<uint8_t, 4>& Ip, const std::array<uint8_t, 4>& Subnet, uint8_t Rf24Channel, uint8_t Rf24DataRate, uint8_t GatewayNodeId);
	bool Begin(std::string Ip, std::string Subnet, uint8_t Rf24Channel, uint8_t Rf24DataRate, uint8_t GatewayNodeId);

	void SetIpAddress(const std::array<uint8_t, 4>& Ip, const std::array<uint8_t, 4>& Subnet);

	static bool IpBytesFromString(std::string IpStr, std::array<uint8_t, 4>& IpBytes);
private:
#ifdef CM_UNIX
	RF24 Radio;
	RF24Network Network;
	RF24Mesh Mesh;
	RF24Gateway Gateway;
	std::mutex Rf24Mutex;

	uint32_t meshTimer = 0;

	bool bIsWorking = false;
	std::atomic<bool> WorkerStopFlag;

	std::thread WorkerThread;
	
	void WorkerMethod();
#endif
};