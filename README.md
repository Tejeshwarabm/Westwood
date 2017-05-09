# Performance evaluation of TCP Westwood and Westwood+ for Multirate 802.11 in ns-3.

## Course Code: CS821

## Overview

TCP Westwood and Westwood+ are TCP extensions proposed mainly for wireless networks [1]. This project aims to evaluate the performance of TCP Westwood and Westwood+ in the presence of different Rate Adaptation Algorithms in ns-3 [2].

### Simulation

Performance evaluation of TCPW and TCPW+ for multirate 802.11 has been provided in

`scratch/wifi.cc`

and should be executed as

 `./waf --run "scratch/wifi --simulationTime=20 --tcpVariant=westwood --raaAlgo=aarf"`
 `./waf --run "scratch/wifi --simulationTime=20 --tcpVariant=westwoodplus --raaAlgo=aarf"`

The above excution steps are for AARF, similarly for all different ARF can be excuted in the same line.

## References

[1] Saverio Mascolo, Claudio Casetti, Mario Gerla, M. Y. Sanadidi, and Ren Wang (2002). TCP Westwood: End-to-End Congestion Control for Wired/Wireless Networks.

[2] Naeem Khademi, Michael Welzl, and Renato Lo Cigno. TCP Westwood: End-to-End Congestion Control for Wired/Wireless Networks
