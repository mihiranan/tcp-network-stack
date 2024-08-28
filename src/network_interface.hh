#pragma once

#include "address.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"

#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

using namespace std;
// A "network interface" that connects IP (the internet layer, or network layer)
// with Ethernet (the network access layer, or link layer).

// This module is the lowest layer of a TCP/IP stack
// (connecting IP with the lower-layer network protocol,
// e.g. Ethernet). But the same module is also used repeatedly
// as part of a router: a router generally has many network
// interfaces, and the router's job is to route Internet datagrams
// between the different interfaces.

// The network interface translates datagrams (coming from the
// "customer," e.g. a TCP/IP stack or router) into Ethernet
// frames. To fill in the Ethernet destination address, it looks up
// the Ethernet address of the next IP hop of each datagram, making
// requests with the [Address Resolution Protocol](\ref rfc::rfc826).
// In the opposite direction, the network interface accepts Ethernet
// frames, checks if they are intended for it, and if so, processes
// the the payload depending on its type. If it's an IPv4 datagram,
// the network interface passes it up the stack. If it's an ARP
// request or reply, the network interface processes the frame
// and learns or replies as necessary.
class NetworkInterface
{
private:
  // Ethernet (known as hardware, network-access, or link-layer) address of the interface
  EthernetAddress ethernet_address_;

  // IP (known as Internet-layer or network-layer) address of the interface
  Address ip_address_;

  struct EthernetInfo
  {
    EthernetAddress eth; // The Ethernet address associated with an IP address
    size_t time;         // The time that has passed since the Ethernet address was mapped
  };

  // The maximum time in milliseconds before an ARP request is resent
  const size_t RESEND_THRESHOLD = 5000;

  // The maximum time in milliseconds before an Ethernet-to-IP mapping is removed
  const size_t MAPPING_THRESHOLD = 30000;

  // Maps an IP address to its corresponding Ethernet information
  unordered_map<uint32_t, EthernetInfo> ethernet_map = {};

  // Maps an IP address to the time that has passed since an ARP request was sent
  unordered_map<uint32_t, size_t> arp_timeout = {};

  // Maps an IP address to a queue of Ethernet frames waiting for an ARP reply
  unordered_map<uint32_t, deque<EthernetFrame>> arp_waiting = {};

  // Queue of Ethernet frames ready to be sent out
  deque<EthernetFrame> send_queue = {};

  // Constructs an Ethernet frame
  EthernetFrame make_eth_frame( const EthernetAddress& src,
                                const EthernetAddress& dst,
                                uint16_t type,
                                std::vector<Buffer> payload );

  // Constructs an ARP message
  ARPMessage make_arp_msg( uint32_t target_ip,
                           uint32_t sender_ip,
                           std::optional<EthernetAddress> target_eth,
                           EthernetAddress& sender_eth,
                           uint16_t opcode );

public:
  // Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer)
  // addresses
  NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address );

  // Access queue of Ethernet frames awaiting transmission
  optional<EthernetFrame> maybe_send();

  // Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination
  // address). Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address
  // for the next hop.
  // ("Sending" is accomplished by making sure maybe_send() will release the frame when next called,
  // but please consider the frame sent as soon as it is generated.)
  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );

  // Receives an Ethernet frame and responds appropriately.
  // If type is IPv4, returns the datagram.
  // If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // If type is ARP reply, learn a mapping from the "sender" fields.
  optional<InternetDatagram> recv_frame( const EthernetFrame& frame );

  // Called periodically when time elapses
  void tick( size_t ms_since_last_tick );
};
