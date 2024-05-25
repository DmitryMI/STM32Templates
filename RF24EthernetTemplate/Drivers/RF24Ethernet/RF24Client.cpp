/*
 RF24Client.cpp - Arduino implementation of a uIP wrapper class.
 Copyright (c) 2014 tmrh20@gmail.com, github.com/TMRh20
 Copyright (c) 2013 Norbert Truchsess <norbert.truchsess@t-online.de>
 All rights reserved.
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */
#include "RF24Ethernet.h"

#define UIP_TCP_PHYH_LEN UIP_LLH_LEN + UIP_IPTCPH_LEN

uip_userdata_t RF24Client::all_data[UIP_CONNS];

/*************************************************************/

RF24Client::RF24Client() : data(NULL) {}

/*************************************************************/

RF24Client::RF24Client(uip_userdata_t* conn_data) : data(conn_data) {}

/*************************************************************/

uint8_t RF24Client::connected()
{
    return (data && (data->packets_in != 0 || (data->state & UIP_CLIENT_CONNECTED))) ? 1 : 0;
}

/*************************************************************/

int RF24Client::connect(IPAddress ip, uint16_t port)
{
#if UIP_ACTIVE_OPEN > 0

    // do{

    stop();
    uip_ipaddr_t ipaddr;
    uip_ip_addr(ipaddr, ip);

    struct uip_conn* conn = uip_connect(&ipaddr, htons(port));

    if (conn)
    {
    #if UIP_CONNECTION_TIMEOUT > 0
        uint32_t timeout = millis();
    #endif

        while ((conn->tcpstateflags & UIP_TS_MASK) != UIP_CLOSED)
        {
            RF24EthernetClass::tick();

            if ((conn->tcpstateflags & UIP_TS_MASK) == UIP_ESTABLISHED)
            {
                data = (uip_userdata_t*)conn->appstate;
                IF_RF24ETHERNET_DEBUG_CLIENT(printf("%ld", millis()); printf(" connected, state: "); printf("%d", data->state); printf(", first packet in: "); printf("%d\n", (int)data->packets_in););
                return 1;
            }

    #if UIP_CONNECTION_TIMEOUT > 0
            if ((millis() - timeout) > UIP_CONNECTION_TIMEOUT)
            {
                conn->tcpstateflags = UIP_CLOSED;
                break;
            }
    #endif
        }
    }
    // delay(25);
    // }while(millis()-timer < 175);

#endif // Active open enabled

    return 0;
}

/*************************************************************/

int RF24Client::connect(const char* host, uint16_t port)
{
    // Look up the host first
    int ret = 0;

#if UIP_UDP
    DNSClient dns;
    IPAddress remote_addr;

    dns.begin(RF24EthernetClass::_dnsServerAddress);
    ret = dns.getHostByName(host, remote_addr);

    if (ret == 1)
    {
    #if defined(ETH_DEBUG_L1) || #defined(RF24ETHERNET_DEBUG_DNS)
        Serial.println(F("*UIP Got DNS*"));
    #endif
        return connect(remote_addr, port);
    }
#else  // ! UIP_UDP
    // Do something with the input parameters to prevent compile time warnings
    if (host) {
    };
    if (port) {
    };
#endif // ! UIP_UDP

#if defined(ETH_DEBUG_L1) || defined(RF24ETHERNET_DEBUG_DNS)
    Serial.println(F("*UIP DNS fail*"));
#endif

    return ret;
}

/*************************************************************/

void RF24Client::stop()
{
    if (data && data->state)
    {

        IF_RF24ETHERNET_DEBUG_CLIENT(printf("%ld", millis()); printf(" before stop(), with data\n"););

        data->packets_in = 0;
        data->dataCnt = 0;

        if (data->state & UIP_CLIENT_REMOTECLOSED)
        {
            data->state = 0;
        }
        else
        {
            data->state |= UIP_CLIENT_CLOSE;
        }

        IF_RF24ETHERNET_DEBUG_CLIENT(printf("after stop()\n"););
    }
    else
    {
        IF_RF24ETHERNET_DEBUG_CLIENT(printf("%ld", millis()); printf(" stop(), data: NULL\n"););
    }

    data = NULL;
    RF24Ethernet.tick();
}

/*************************************************************/

// the next function allows us to use the client returned by
// EthernetServer::available() as the condition in an if-statement.
bool RF24Client::operator==(const RF24Client& rhs)
{
    return data && rhs.data && (data == rhs.data);
}

/*************************************************************/

RF24Client::operator bool()
{
    Ethernet.tick();
    return data && (!(data->state & UIP_CLIENT_REMOTECLOSED) || data->packets_in != 0);
}

/*************************************************************/

size_t RF24Client::write(uint8_t c)
{
    return _write(data, &c, 1);
}

/*************************************************************/

size_t RF24Client::write(const uint8_t* buf, size_t size)
{
    return _write(data, buf, size);
}

/*************************************************************/

size_t RF24Client::_write(uip_userdata_t* u, const uint8_t* buf, size_t size)
{

    size_t total_written = 0;
    size_t payloadSize = rf24_min(size, UIP_TCP_MSS);

test2:

    RF24EthernetClass::tick();
    if (u && !(u->state & (UIP_CLIENT_CLOSE | UIP_CLIENT_REMOTECLOSED)) && u->state & (UIP_CLIENT_CONNECTED))
    {

        if (u->out_pos + payloadSize > UIP_TCP_MSS || u->hold)
        {
            goto test2;
        }

        // IF_RF24ETHERNET_DEBUG_CLIENT(printf("\n"); printf("%ld", millis()); printf(" UIPClient.write: writePacket("); printf("%d", u->packets_out); printf(") pos: "); printf("%lu", u->out_pos); printf(", buf["); printf("%d", size - total_written); printf("]: '"); /*Serial.write((uint8_t*)buf + total_written, payloadSize);*/ printf("'\n"););
        IF_RF24ETHERNET_DEBUG_CLIENT(::printf("\n%ld UIPClient.write: writePacket(%d) pos: %u, buf[%d]: XXX\n", millis(), u->packets_out, u->out_pos, size - total_written););
        memcpy(u->myData + u->out_pos, buf + total_written, payloadSize);
        u->packets_out = 1;
        u->out_pos += payloadSize;

        total_written += payloadSize;

        if (total_written < size)
        {
            size_t remain = size - total_written;
            payloadSize = rf24_min(remain, UIP_TCP_MSS);

            // RF24EthernetClass::tick();
            goto test2;
        }
        return u->out_pos;
    }
    u->hold = false;
    return -1;
}

/*************************************************************/

void uip_log(char* msg)
{
    // Serial.println();
    // Serial.println("** UIP LOG **");
    // Serial.println(msg);
    if (msg)
    {
    };
}

/*************************************************************/

void serialip_appcall(void)
{
    uip_userdata_t* u = (uip_userdata_t*)uip_conn->appstate;

    /*******Connected**********/
    if (!u && uip_connected())
    {
        // IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(); Serial.print(millis()); Serial.println(F(" UIPClient uip_connected")););
        IF_RF24ETHERNET_DEBUG_CLIENT(printf("%ld UIPClient uip_connected", millis()));

        u = (uip_userdata_t*)EthernetClient::_allocateData();

        if (u)
        {
            uip_conn->appstate = u;
            // IF_RF24ETHERNET_DEBUG_CLIENT(Serial.print(F("UIPClient allocated state: ")); Serial.println(u->state, BIN););
            IF_RF24ETHERNET_DEBUG_CLIENT(printf("UIPClient allocated state: %x\n", u->state););
        }
        else
        {
            // IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(F("UIPClient allocation failed")););
        	IF_RF24ETHERNET_DEBUG_CLIENT(printf("UIPClient allocation failed\n"););
        }
    }

#if UIP_CONNECTION_TIMEOUT > 0
    if (u && u->connectTimeout > 0) {
        if (millis() - u->connectTimer > u->connectTimeout) {
            u->state |= UIP_CLIENT_CLOSE;
            u->connectTimer = millis();
            // IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(); Serial.print(millis()); Serial.println("UIP Client close(timeout)"););
            IF_RF24ETHERNET_DEBUG_CLIENT(printf("%ld UIP Client close(timeout)\n", millis()););
        }
    }
#endif

    /*******User Data RX**********/
    if (u)
    {
        if (uip_newdata())
        {
            // IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(); Serial.print(millis()); Serial.print(F(" UIPClient uip_newdata, uip_len:")); Serial.println(uip_len););
            IF_RF24ETHERNET_DEBUG_CLIENT(printf("%ld UIPClient uip_newdata, uip_len: %d\n", millis(), uip_len););
#if UIP_CONNECTION_TIMEOUT > 0
            u->connectTimer = millis();
#endif

            if (u->sent)
            {
                u->hold = (u->out_pos = (u->windowOpened = (u->packets_out = false)));
            }
            if (uip_len && !(u->state & (UIP_CLIENT_CLOSE | UIP_CLIENT_REMOTECLOSED)))
            {
                uip_stop();
                u->state &= ~UIP_CLIENT_RESTART;
                u->windowOpened = false;
                u->restartTime = millis();
                memcpy(&u->myData[u->dataPos + u->dataCnt], uip_appdata, uip_datalen());
                u->dataCnt += uip_datalen();

                u->packets_in = 1;
            }
            goto finish;
        }

        /*******Closed/Timed-out/Aborted**********/
        // If the connection has been closed, save received but unread data.
        if (uip_closed() || uip_timedout() || uip_aborted())
        {
            // IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(); Serial.print(millis()); Serial.println(F(" UIPClient uip_closed")););
            IF_RF24ETHERNET_DEBUG_CLIENT(printf("%ld UIPClient uip_closed\n", millis()););

            // drop outgoing packets not sent yet:
            u->packets_out = 0;

            if (u->packets_in)
            {
                ((uip_userdata_closed_t*)u)->lport = uip_conn->lport;
                u->state |= UIP_CLIENT_REMOTECLOSED;
                IF_RF24ETHERNET_DEBUG_CLIENT(printf("UIPClient close 1\n"););
            }
            else
            {
                IF_RF24ETHERNET_DEBUG_CLIENT(printf("UIPClient close 2\n"););
                u->state = 0;
            }

            IF_RF24ETHERNET_DEBUG_CLIENT(printf("after UIPClient uip_closed\n"););
            uip_conn->appstate = NULL;
            goto finish;
        }

        /*******ACKED**********/
        if (uip_acked())
        {
            //IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(); Serial.print(millis()); Serial.println(F(" UIPClient uip_acked")););

        	IF_RF24ETHERNET_DEBUG_CLIENT(printf("\n%ld UIPClient uip_acked\n", millis()););
        	u->state &= ~UIP_CLIENT_RESTART;
            u->hold = (u->out_pos = (u->windowOpened = (u->packets_out = false)));
            u->restartTime = millis();
#if UIP_CONNECTION_TIMEOUT > 0
            u->connectTimer = millis();
#endif
        }

        /*******Polling**********/
        if (uip_poll() || uip_rexmit())
        {
            // IF_RF24ETHERNET_DEBUG_CLIENT( Serial.println(); Serial.println(F("UIPClient uip_poll")); );

            if (u->packets_out != 0)
            {
                uip_len = u->out_pos;
                uip_send(u->myData, u->out_pos);
                u->hold = true;
                u->sent = true;
                goto finish;
            }
            else
                // Restart mechanism to keep connections going
                // Only call this if the TCP window has already been re-opened, the connection is being polled, but no data
                // has been acked
                if (!(u->state & (UIP_CLIENT_CLOSE | UIP_CLIENT_REMOTECLOSED)))
            {

                if (u->windowOpened == true && u->state & UIP_CLIENT_RESTART && millis() - u->restartTime > u->restartInterval)
                {
                    u->restartTime = millis();
#if defined RF24ETHERNET_DEBUG_CLIENT || defined ETH_DEBUG_L1
                    printf("\n");
                    printf("%ld", millis());
    #if UIP_CONNECTION_TIMEOUT > 0
                    //Serial.print(F(" UIPClient Re-Open TCP Window, time remaining before abort: "));
                    //Serial.println(UIP_CONNECTION_TIMEOUT - (millis() - u->connectTimer));
                    printf(" UIPClient Re-Open TCP Window, time remaining before abort: %ld\n", UIP_CONNECTION_TIMEOUT - (millis() - u->connectTimer));
    #endif
#endif
                    u->restartInterval += 500;
                    u->restartInterval = rf24_min(u->restartInterval, 7000);
                    uip_restart();
                }
            }
        }

        /*******Close**********/
        if (u->state & UIP_CLIENT_CLOSE)
        {
            //IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(); Serial.print(millis()); Serial.println(F(" UIPClient state UIP_CLIENT_CLOSE")););
        	IF_RF24ETHERNET_DEBUG_CLIENT(printf("%ld UIPClient state UIP_CLIENT_CLOSE\n", millis()););
            if (u->packets_out == 0)
            {
                u->state = 0;
                uip_conn->appstate = NULL;
                uip_close();
                // IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(F("no blocks out -> free userdata")););
                IF_RF24ETHERNET_DEBUG_CLIENT(printf("no blocks out -> free userdata\n"););
            }
            else
            {
                uip_stop();
                // IF_RF24ETHERNET_DEBUG_CLIENT(Serial.println(F("blocks outstanding transfer -> uip_stop()")););
                IF_RF24ETHERNET_DEBUG_CLIENT(printf("blocks outstanding transfer -> uip_stop()\n"););
            }
        }
finish:;

        if (u->state & UIP_CLIENT_RESTART && !u->windowOpened)
        {
            if (!(u->state & (UIP_CLIENT_CLOSE | UIP_CLIENT_REMOTECLOSED)))
            {
                uip_restart();
#if defined ETH_DEBUG_L1
                Serial.println();
                Serial.print(millis());
                Serial.println(F(" UIPClient Re-Open TCP Window"));
#endif
                u->windowOpened = true;
                u->restartInterval = UIP_WINDOW_REOPEN_DELAY; //.75 seconds
                u->restartTime = millis();
            }
        }
    }
}

/*******************************************************/

uip_userdata_t* RF24Client::_allocateData()
{
    for (uint8_t sock = 0; sock < UIP_CONNS; sock++)
    {
        uip_userdata_t* data = &RF24Client::all_data[sock];
        if (!data->state)
        {
            data->state = sock | UIP_CLIENT_CONNECTED;
            data->packets_in = 0;
            data->packets_out = 0;
            data->dataCnt = 0;
            data->dataPos = 0;
            data->out_pos = 0;
            data->hold = 0;
#if (UIP_CONNECTION_TIMEOUT > 0)
            data->connectTimer = millis();
            data->connectTimeout = UIP_CONNECTION_TIMEOUT;
#endif
            return data;
        }
    }
    return NULL;
}

int RF24Client::waitAvailable(uint32_t timeout)
{
    uint32_t start = millis();
    while (available() < 1)
    {
        if (millis() - start > timeout)
        {
            return 0;
        }
        RF24Ethernet.tick();
    }
    return available();
}

/*************************************************************/

int RF24Client::available()
{
    RF24Ethernet.tick();
    if (*this)
    {
        return _available(data);
    }
    return 0;
}

/*************************************************************/

int RF24Client::_available(uip_userdata_t* u)
{
    if (u->packets_in)
    {
        return u->dataCnt;
    }
    return 0;
}

int RF24Client::read(uint8_t* buf, size_t size)
{
    if (*this)
    {
        if (!data->packets_in)
        {
            return -1;
        }

        size = rf24_min(data->dataCnt, size);
        memcpy(buf, &data->myData[data->dataPos], size);
        data->dataCnt -= size;

        data->dataPos += size;

        if (!data->dataCnt)
        {
            data->packets_in = 0;
            data->dataPos = 0;

            if (uip_stopped(&uip_conns[data->state & UIP_CLIENT_SOCKETS]) && !(data->state & (UIP_CLIENT_CLOSE | UIP_CLIENT_REMOTECLOSED)))
            {
                data->state |= UIP_CLIENT_RESTART;
                data->restartTime = 0;

                IF_ETH_DEBUG_L2(Serial.print(F("UIPClient set restart ")); Serial.println(data->state & UIP_CLIENT_SOCKETS); Serial.println(F("**")); Serial.println(data->state, BIN); Serial.println(F("**")); Serial.println(UIP_CLIENT_SOCKETS, BIN); Serial.println(F("**")););
            }
            else
            {
                IF_ETH_DEBUG_L2(Serial.print(F("UIPClient stop?????? ")); Serial.println(data->state & UIP_CLIENT_SOCKETS); Serial.println(F("**")); Serial.println(data->state, BIN); Serial.println(F("**")); Serial.println(UIP_CLIENT_SOCKETS, BIN); Serial.println(F("**")););
            }

            if (data->packets_in == 0)
            {
                if (data->state & UIP_CLIENT_REMOTECLOSED)
                {
                    data->state = 0;
                    data = NULL;
                }
            }
        }
        return size;
    }

    return -1;
}

/*************************************************************/

int RF24Client::read()
{
    uint8_t c;
    if (read(&c, 1) < 0)
        return -1;
    return c;
}

/*************************************************************/

int RF24Client::peek()
{
    if (available())
    {
        return data->myData[data->dataPos];
    }
    return -1;
}

/*************************************************************/

void RF24Client::flush()
{
    if (*this)
    {
        data->packets_in = 0;
        data->dataCnt = 0;
    }
}
