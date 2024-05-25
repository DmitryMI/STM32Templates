# sudo ip tuntap add dev tun_nrf24 mode tun user pi multi_queue
ifconfig tun_nrf24 10.10.0.1/24
sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -A POSTROUTING -j MASQUERADE