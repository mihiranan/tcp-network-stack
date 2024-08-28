#include "reassembler.hh"

using namespace std;

/*
This method takes in a data fragment, its starting index, a flag indicating whether it is the last substring in the
stream, and a reference to the Writer object. It reassembles the data stream and writes the continuous segments to
the output.
 */
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  uint64_t out_of_bounds
    = output.bytes_pushed() + output.available_capacity(); // The out-of-bounds index for the data
  uint64_t internal_first
    = first_index
      - output.bytes_pushed(); // The Reassembler's first index relative to the bytes already pushed to the output

  // If the data is empty or starts beyond the available capacity, it is not valid and there is nothing to insert
  if ( data.size() == 0 || first_index >= out_of_bounds ) {
    if ( is_last_substring ) {
      last = true; // Set the end of stream flag
    }
    if ( last && bytes_pending() == 0 ) {
      output.close(); // Close the Writer
    }
    return;
  }

  uint64_t needed_space = internal_first + data.size(); // Needed space for the incoming data

  // If the current vector size is smaller than the needed space, resize the vectors accordingly
  if ( bytes.size() <= ( needed_space ) ) {
    size_t new_size = min( needed_space, output.available_capacity() );
    bytes.resize( new_size );
    filled.resize( new_size, false );
  }

  // Copy the data into the 'bytes' vector and update the 'filled' vector
  for ( size_t i = 0; i < data.size(); i++ ) {
    if ( internal_first + i >= bytes.size() ) {
      continue;
    }
    if ( !filled[internal_first + i] ) {
      pending++; // Keeping track of the pending bytes in Reassembler
    }
    bytes[internal_first + i] = data[i];
    filled[internal_first + i] = true;
  }

  string package; // String to store the continuous bytes for the output
  for ( size_t i = 0; filled[i] && i < bytes.size(); i++ ) {
    package += bytes[i]; // Concatenate continuous bytes to the package
  }
  output.push( package ); // Push the completed string to the Writer

  // Remove the continuous bytes and corresponding filled elements from the vectors
  filled.erase( filled.begin(), filled.begin() + package.size() );
  bytes.erase( bytes.begin(), bytes.begin() + package.size() );

  pending -= package.size(); // Keeping track of the pending bytes in Reassembler

  if ( is_last_substring ) {
    last = true; // Set the end of stream flag
  }
  if ( last && bytes_pending() == 0 ) {
    output.close(); // Close the Writer
  }
}

// This method returns the number of bytes pending reassembly, which is kept track of in the insert() method.
uint64_t Reassembler::bytes_pending() const
{
  return pending;
}
