# mcastit - Multicast Interface Tool (Linux)

## Why mcastit
Why do you need yet another multicast tool while there exist various high quality networking tools that (you think) could do the job? Especially this tool currently only works with linux?

Well, I have thought about that too. But I'm unable to find one that could bind exactly to the network interface, not just the interface address. What if you have 2 interfaces with the same IP address? Which one would it bind to? I know that's a bad practice to have that setup but, well...it is possible, right?

Therefore, may I introduce you mcastit: a network tool that specializes in network interface name instead of interface IP address :D

## Manual
### Feature:

 * IPv4 & IPv6
 * Multicast sender with/without loopback
 * Multiple network interface support

### Usage

```bash
./mcastit [options] [iface1 iface2 ...]
    Option:
    -6                 use IPv6
    -m {mcast address} multicast address, default: 239.192.0.123 and FFFE::1:FF47:0(v6)
    -p {port}          multicast port, default: 12321

    -l                 listen mode
    -o                 turn off loop back on sender
    -h                 This message
```
### Examples
Sender on interface docker0 & wlp4s0 for multicast address 224.1.1.1 port 12321:
```
./mcastit -m 224.1.1.1 -p 12321 wlp4s0 docker0
MCAST with IPV4 224.1.1.1 port 12321
Sending with loopback
==============================================================
[OK] sent to [wlp4s0 (192.168.0.12)] bytes: 37
[OK] sent to [docker0 (172.17.0.1)] bytes: 36
```

Listener on interface docker0 & wlp4s0 for multicast address 224.1.1.1 port 12321:
```
./mcastit -l -m 224.1.1.1 -p 12321 wlp4s0 docker0
MCAST with IPV4 224.1.1.1 port 12321
Listening ...
Interface wlp4s0 (192.168.0.12) [OK]
Interface docker0 (172.17.0.1) [OK]
==============================================================
192.168.0.12 - <Sender info: wlp4s0 (192.168.0.12)>
172.17.0.1 - <Sender info: docker0 (172.17.0.1)>
```

## Other useful multicast related tools

 * [mtools](https://github.com/troglobit/mtools) good general IPv4 mcast testing tool with interval & TTL setting
 * [omping](https://github.com/troglobit/omping) feature-rich mcast suite that has everything you need (except some features in mcastit:) )
 * [iperf](https://iperf.fr/iperf-doc.php) multicast/othercast benchmark tool
 
