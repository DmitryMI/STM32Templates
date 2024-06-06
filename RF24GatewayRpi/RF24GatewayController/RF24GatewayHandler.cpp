#include "RF24GatewayHandler.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <ctype.h>
#include <string>

bool RF24GatewayHandler::IpBytesFromString(std::string IpStr, std::array<uint8_t, 4>& IpBytes)
{
    std::array<std::string, 4> OctetsStrings;
    int OctetIndex = 0;
    for (int i = 0; i < IpStr.size(); i++)
    {
        if (IpStr[i] == '.')
        {
            OctetIndex++;
            if (OctetIndex == 4)
            {
                return false;
            }
        }
        else if (isdigit(IpStr[i]))
        {
            OctetsStrings[OctetIndex] += IpStr[i];
        }
        else
        {
            return false;
        }
    }

    for (int i = 0; i < OctetsStrings.size(); i++)
    {
        IpBytes[i] = std::stoi(OctetsStrings[i]);
    }

    return true;
}

#ifdef CM_UNIX
RF24GatewayHandler::RF24GatewayHandler(uint16_t CePin, uint16_t CsnPin) : Radio(CePin, CsnPin), Network(Radio), Mesh(Radio, Network), Gateway(Radio, Network, Mesh)
{

}
#endif

#ifdef CM_WIN32
RF24GatewayHandler::RF24GatewayHandler(uint16_t CePin, uint16_t CsnPin)
{

}
#endif

RF24GatewayHandler::~RF24GatewayHandler()
{
#ifdef CM_UNIX
    WorkerStopFlag.store(true);
    if (WorkerThread.joinable())
    {
        WorkerThread.join();
    }
#endif
}

bool RF24GatewayHandler::Begin(std::string Ip, std::string Subnet)
{
    std::array<uint8_t, 4> IpBytes;
    std::array<uint8_t, 4> SubnetBytes;
    if (!IpBytesFromString(Ip, IpBytes))
    {
        spdlog::error("Failed to parse IP string: {}", Ip);
        return false;
    }
    if (!IpBytesFromString(Subnet, SubnetBytes))
    {
        spdlog::error("Failed to parse Subnet string: {}", Subnet);
        return false;
    }
    spdlog::info("Gateway IP parsed: {}.{}.{}.{}", IpBytes[0], IpBytes[1], IpBytes[2], IpBytes[3]);
    spdlog::info("Gateway Subnet parsed: {}.{}.{}.{}", SubnetBytes[0], SubnetBytes[1], SubnetBytes[2], SubnetBytes[3]);
    return Begin(IpBytes, SubnetBytes);
}

bool RF24GatewayHandler::Begin(const std::array<uint8_t, 4>& Ip, const std::array<uint8_t, 4>& Subnet)
{
    spdlog::info(
        "Starting RF24GatewayHandler with IP {}.{}.{}.{} and Subnet {}.{}.{}.{}...",
        Ip[0], Ip[1], Ip[2], Ip[3],
        Subnet[0], Subnet[1], Subnet[2], Subnet[3]
        );

#ifdef CM_UNIX
    if (bIsWorking)
    {
        spdlog::info("RF24GatewayHandler already running, changing IP.");
        SetIpAddress(Ip, Subnet);
        return true;
    }

    if (!Radio.begin())
    {
        spdlog::error("Failed to begin Radio");
        return false;
    }
	
    Gateway.begin(0, 97, RF24_250KBPS);
	
    WorkerThread = std::thread(&RF24GatewayHandler::WorkerMethod, this);
    
    spdlog::info("WorkerThread started");
    return true;
#endif
    return true;
}

void RF24GatewayHandler::SetIpAddress(const std::array<uint8_t, 4>& Ip, const std::array<uint8_t, 4>& Subnet)
{
#ifdef CM_UNIX
    std::lock_guard<std::mutex> lock(Rf24Mutex);
    char IpBuffer[4];
    char SubnetBuffer[4];

    memcpy(IpBuffer, Ip.data(), Ip.size());
    memcpy(SubnetBuffer, Subnet.data(), Subnet.size());

    Gateway.setIP(IpBuffer, SubnetBuffer);
    spdlog::error("IP set to {}.{}.{}.{}, Subnet set to {}.{}.{}.{}", 
        Ip[0], Ip[1], Ip[2], Ip[3],
        Subnet[0], Subnet[1], Subnet[2], Subnet[3]
    );
#endif
}

#ifdef CM_UNIX
void RF24GatewayHandler::WorkerMethod()
{
    while (!WorkerStopFlag.load())
    {

        // The gateway handles all IP traffic (marked as EXTERNAL_DATA_TYPE) and passes it to the associated network interface
        // RF24Network user payloads are loaded into the user cache
        Gateway.update();
        if (Network.available())
        {
            RF24NetworkHeader header;
            size_t size = Network.peek(header);
            uint8_t buf[size];
            Network.read(header, &buf, size);
            spdlog::info("Received Network Message, type: {} id {} from {}\n", header.type, header.id, Mesh.getNodeID(header.from_node));

            RF24NetworkFrame frame = RF24NetworkFrame(header, buf, size);
            Gateway.sendUDP(Mesh.getNodeID(header.from_node), frame);
        }
        delay(2);

        if (millis() - meshTimer > 30000 && Mesh.getNodeID())
        { //Every 30 seconds, test mesh connectivity
            meshTimer = millis();
            if (!Mesh.checkConnection())
            {
                spdlog::info("Mesh.checkConnection() returned false");
                //refresh the network address
                Mesh.renewAddress();
                spdlog::info("Mesh address renewed");
            }
        }
        //This section checks for failures detected by RF24 & RF24Network as well as
        //checking for deviations from the default configuration (1MBPS data rate)
        //The mesh is restarted on failure and failure count logged to failLog.txt
        //This makes the radios hot-swappable, disconnect & reconnect as desired, it should come up automatically
        if (Radio.failureDetected > 0)
        {
            Radio.failureDetected = 0;
            std::ofstream myFile;
            
            spdlog::error("RF24 failure detected.");

            delay(500);
            Mesh.begin();
        }
    }
}

#endif