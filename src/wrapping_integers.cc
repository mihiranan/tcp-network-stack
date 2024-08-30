#include "wrapping_integers.hh"
#include <cstdlib>
#include <algorithm>

using namespace std;

// The "wrap" method takes an unsigned 64-bit integer "n" and a Wrap32 instance "zero_point" as input,
// and returns a Wrap32 instance representing the wrapped value of "n" with respect to "zero_point"
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + n;
}

// The "unwrap" method takes a Wrap32 instance "zero_point" and an unsigned 64-bit integer "checkpoint" as input,
// and returns an unsigned 64-bit integer representing the unwrapped value of the current object with respect to
// "zero_point" and "checkpoint"
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Calculate the reference value "ref" as 2^32
  uint64_t ref = 1UL << 32;

  // Calculate the sequence number as the difference between the raw values of the current object and "zero_point"
  uint32_t seqno = raw_value_ - zero_point.raw_value_;

  // Calculate the "low" value by adding "seqno" to the "checkpoint" minus the remainder of the division of
  // "checkpoint" by "ref"
  uint64_t low = seqno + ( checkpoint - ( checkpoint % ref ) );

  // If "low" is greater than or equal to "checkpoint", subtract "ref" from "low"
  if ( low >= checkpoint ) {
    low -= ref;
  }

  // The absolute difference "diff" between "low" and "checkpoint"
  uint64_t diff = max( low, checkpoint ) - min( low, checkpoint );

  if ( diff <= ( ref >> 1 ) ) {
    return low;
  }
  return low + ref;
}
