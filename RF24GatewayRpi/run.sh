#!/usr/bin/sh

sudo ip tuntap add dev tun_nrf24 mode tun user pi multi_queue
sudo ifconfig tun_nrf24 10.10.2.0/24
sudo sysctl -w net.ipv4.ip_forward=1

sudo iptables -A FORWARD -i tun_nrf24 -o wlan0 -m state --state ESTABLISHED,RELATED -j ACCEPT
sudo iptables -A FORWARD -i wlan0 -o tun_nrf24 -j ACCEPT
sudo iptables -t nat -A POSTROUTING -o tun_nrf24 -j MASQUERADE
sudo iptables -t nat -A POSTROUTING -j MASQUERADE

mkdir -p logs 

RF24GatewayController/RF24GatewayController --log-dir logs