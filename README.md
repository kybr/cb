# cb: a framework for efficient\* distribution of ephemeral\*\* data

\* if optimistic/unreliable

\*\* mostly ephemeral

Your average wired network is very reliable. Unless something is very wrong, we
may trust a LAN to deliver [UDP][] packets. Your average gigabit ethernet
switch will efficiently [broadcast][] or [multicast][] these packets with very
low latency and each will arrive at its destination at more or less the same
time thanks to "[switch fabric][]". However, the size of UDP packets is
effectively limited to 65507 bytes and the size of ethernet packets is
effectively limited to 1500 (~9000 in some cases) bytes. Your operating system
(and maybe your hardware) will "fragment" your packets that are larger than
1500 and craft many packets of 1500 bytes or smaller. The remote machines will
reassmble these packets and deliver them to the software layer.

This framework does something similar. It chops your data into pieces before
delivering them to your operating system for broadcast via UDP. On the remote
machines, it receives, collects, and reassmbles the pieces to reconstruct your
data. *You may choose how big the pieces are.* 

**Of course, there are lots of caveats and/or justifications I should add to
the statements above (and I will), but for now, just deal with it.**

### Questions?

- Why not TCP?
- Why not [PGM]?
- Why not use raw IP sockets?
- Why use libuv?
- What about [ENet]?


[UDP]: https://www.ietf.org/rfc/rfc768.txt
[broadcast]: https://en.wikipedia.org/wiki/Broadcasting_(networking)
[multicast]: https://en.wikipedia.org/wiki/Multicast
[switch fabric]: http://etherealmind.com/what-is-the-definition-of-switch-fabric
[PGM]: https://en.wikipedia.org/wiki/Pragmatic_General_Multicast
[2]: https://en.wikipedia.org/wiki/Crossbar_switch
[ENet]: http://enet.bespin.org
