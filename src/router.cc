#include "router.hh"

#include "address.hh"

using namespace std;

void Router::process_datagram( InternetDatagram& datagram, const RouteInfo& route_info )
{
  datagram.header.ttl--;
  datagram.header.compute_checksum();

  if ( route_info.next_hop.has_value() ) {
    // Send the datagram to the specified next hop on the corresponding interface
    interface( route_info.interface_num ).send_datagram( datagram, route_info.next_hop.value() );
  } else {
    Address next_hop = Address::from_ipv4_numeric( datagram.header.dst );
    // Send the datagram to the destination address on the corresponding interface
    interface( route_info.interface_num ).send_datagram( datagram, next_hop );
  }
}

bool Router::is_matching_prefix( const InternetDatagram& datagram, const RouteInfo& route_info )
{
  // Check if the route prefix length is 0, which means it matches all addresses
  if ( route_info.prefix_length == 0 )
    return true;

  // Create a bitmask based on the prefix length
  uint32_t mask = 0xFFFFFFFF << ( 32 - route_info.prefix_length );

  // Apply the bitmask to the destination address of the datagram
  // and compare it with the route prefix to check for a match
  return ( datagram.header.dst & mask ) == route_info.route_prefix;
}

void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  // Create a RouteInfo object with the provided parameters
  RouteInfo route_info = { route_prefix, prefix_length, next_hop, interface_num };

  // Add the route_info to the routing_table
  routing_table.emplace_back( route_info );

  // Sort the routing_table based on prefix_length in descending order
  sort( routing_table.begin(), routing_table.end(), compare_routes );
}

void Router::route()
{
  for ( AsyncNetworkInterface& interface : interfaces_ ) { // Iterate over each network interface
    while ( auto maybe_datagram
            = interface.maybe_receive() ) { // Process received datagrams until there are no more
      InternetDatagram datagram = maybe_datagram.value();

      // Iterate over each route in the routing table
      for ( const RouteInfo& route_info : routing_table ) {
        if ( is_matching_prefix(
               datagram, route_info ) ) {  // Check if the route matches the destination address of the datagram
          if ( datagram.header.ttl > 1 ) { // Check if the TTL of the datagram allows further forwarding
            process_datagram( datagram, route_info ); // Helper function all
          }
          break; // Exit the loop after finding a matching route
        }
      }
    }
  }
}
