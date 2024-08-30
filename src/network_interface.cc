#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// Construct an Ethernet frame
EthernetFrame NetworkInterface::make_eth_frame( const EthernetAddress& src,
                                                const EthernetAddress& dst,
                                                uint16_t type,
                                                std::vector<Buffer> payload )
{
  EthernetFrame frame;
  frame.header.src = src;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = payload;
  return frame;
}

// Constructs an ARP (Address Resolution Protocol) message.
ARPMessage NetworkInterface::make_arp_msg( uint32_t target_ip,
                                           uint32_t sender_ip,
                                           std::optional<EthernetAddress> target_eth,
                                           EthernetAddress& sender_eth,
                                           uint16_t opcode )
{
  ARPMessage msg;
  msg.target_ip_address = target_ip;
  msg.sender_ip_address = sender_ip;
  msg.sender_ethernet_address = sender_eth;
  if ( target_eth ) {
    msg.target_ethernet_address = target_eth.value();
  }
  msg.opcode = opcode;
  return msg;
}

void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t next_hop_numeric = next_hop.ipv4_numeric();

  EthernetFrame eth_frame;
  EthernetFrame request_frame
    = make_eth_frame( ethernet_address_, ETHERNET_BROADCAST, EthernetHeader::TYPE_IPv4, serialize( dgram ) );

  // Check if Ethernet address for next hop is known
  if ( ethernet_map.find( next_hop_numeric ) != ethernet_map.end() ) {
    // If known, create frame and push to send queue
    eth_frame = make_eth_frame(
      ethernet_address_, ethernet_map[next_hop_numeric].eth, EthernetHeader::TYPE_IPv4, serialize( dgram ) );
    send_queue.push_back( eth_frame );
  } else {
    // If unknown, create ARP request for next hop
    ARPMessage arp_msg = make_arp_msg(
      next_hop_numeric, ip_address_.ipv4_numeric(), {}, ethernet_address_, ARPMessage::OPCODE_REQUEST );
    eth_frame
      = make_eth_frame( ethernet_address_, ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP, serialize( arp_msg ) );

    // If no previous ARP request pending, send request and record in arp_timeout
    if ( arp_timeout.find( next_hop_numeric ) == arp_timeout.end() ) {
      send_queue.push_back( eth_frame );
      arp_timeout[next_hop_numeric] = 0;
    }
    // Save the original datagram in arp_waiting queue until ARP reply is received
    arp_waiting[next_hop_numeric].push_back( request_frame );
  }
}

optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Only process frames with broadcast destination or those addressed to our own Ethernet address
  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ ) {
    return {};
  }

  // Try to parse frame payload as an IPv4 datagram
  InternetDatagram ipv4_datagram;
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 && parse( ipv4_datagram, frame.payload ) ) {
    // If successful, return the parsed datagram
    return std::optional<InternetDatagram> { ipv4_datagram };
  }

  // Try to parse frame payload as an ARP message
  ARPMessage arp_message;
  if ( frame.header.type != EthernetHeader::TYPE_ARP || !parse( arp_message, frame.payload ) ) {
    // If unsuccessful (either not an ARP message or parsing failed), return nullopt
    return std::nullopt;
  }

  // Update the Ethernet information associated with the sender IP in the Ethernet map
  EthernetInfo& ethernet_info = ethernet_map[arp_message.sender_ip_address];
  ethernet_info.eth = arp_message.sender_ethernet_address;
  ethernet_info.time = 0;

  // If there are any frames waiting for an ARP reply from this sender IP
  if ( arp_waiting.count( arp_message.sender_ip_address ) ) {
    // Update destination Ethernet address of all waiting frames to the sender's Ethernet address and push to send
    // queue
    for ( EthernetFrame& queued_frame : arp_waiting[arp_message.sender_ip_address] ) {
      queued_frame.header.dst = ethernet_info.eth;
      send_queue.push_back( queued_frame );
    }
    // Clear the list of waiting frames for this sender IP
    arp_waiting[arp_message.sender_ip_address].clear();
  }

  // If the target IP of the ARP message matches our IP and the opcode indicates an ARP request
  if ( arp_message.target_ip_address == ip_address_.ipv4_numeric()
       && arp_message.opcode == ARPMessage::OPCODE_REQUEST ) {
    // Construct an ARP reply and push it to the send queue
    ARPMessage arp_reply_message = make_arp_msg( arp_message.sender_ip_address,
                                                 arp_message.target_ip_address,
                                                 { frame.header.src },
                                                 ethernet_address_,
                                                 ARPMessage::OPCODE_REPLY );
    EthernetFrame arp_reply_frame = make_eth_frame( ethernet_address_,
                                                    arp_message.sender_ethernet_address,
                                                    EthernetHeader::TYPE_ARP,
                                                    serialize( arp_reply_message ) );
    send_queue.push_back( arp_reply_frame );
    arp_timeout[arp_message.target_ip_address] = 0; // Reset the ARP timeout for the target IP
  }

  // If we got this far, there's no IPv4 datagram to return, so return nullopt
  return std::nullopt;
}

void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  if ( !ethernet_map.empty() ) {
    std::unordered_map<uint32_t, EthernetInfo> temp_map; // Temporary map to hold updated entries

    for ( const auto& pair : ethernet_map ) {
      // If the current mapping's time is within the 30-second threshold, update and add it to temp_map
      if ( pair.second.time + ms_since_last_tick <= MAPPING_THRESHOLD ) {
        EthernetInfo new_info = { pair.second.eth, pair.second.time + ms_since_last_tick };
        temp_map.insert( { pair.first, new_info } );
      } else {
        // If the mapping is older than the threshold, remove it from arp_waiting as we are no longer waiting
        arp_waiting.erase( pair.first );
      }
    }
    ethernet_map.swap( temp_map ); // Replace ethernet_map with the updated mappings from temp_map
  }

  // If arp_timeout is not empty, update it
  if ( !arp_timeout.empty() ) {
    std::unordered_map<uint32_t, size_t> temp_arp; // Temporary map to hold updated entries again
    for ( auto pair : arp_timeout ) {
      // If the current mapping's time is within the 5-second threshold, update and add it to temp_arp
      if ( pair.second + ms_since_last_tick <= RESEND_THRESHOLD ) {
        temp_arp.insert( { pair.first, pair.second + ms_since_last_tick } );
      }
    }
    arp_timeout.swap( temp_arp ); // Replace arp_timeout with the updated mappings from temp_arp again
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  // If there are frames in the send_queue
  if ( !send_queue.empty() ) {
    EthernetFrame frame = send_queue.front(); // Get the next frame in the queue
    send_queue.pop_front();                   // Remove the sent frame from the queue
    return { frame };
  }
  return {}; // If no frames are ready to be sent, return an empty optional
}
