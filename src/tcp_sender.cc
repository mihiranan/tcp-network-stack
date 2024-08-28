#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <random>

using namespace std;

// Constructor that initializes the Initial Sequence Number (ISN) and initial retransmission timeout.
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

// Calculates the total number of sequence numbers in flight.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  uint64_t in_flight = 0;
  for ( auto msg : sent_segs ) {
    in_flight += msg.sequence_length();
  }
  return in_flight;
}

// Returns the number of consecutive retransmissions.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmissions;
}

// If there are outstanding segments, returns the next one to be sent.
optional<TCPSenderMessage> TCPSender::maybe_send()
{
  if ( outstanding_segs.empty() )
    return std::nullopt;
  TCPSenderMessage msg = outstanding_segs.front();
  outstanding_segs.pop_front();
  return msg;
}

// Handles data to be sent over the network by creating and storing TCPSenderMessage objects.
void TCPSender::push( Reader& outbound_stream )
{
  // Calculate the current window size.
  uint64_t actual_window = ( window == 0 ? 1 : window );
  actual_window
    = ( actual_window >= sequence_numbers_in_flight() ) ? actual_window - sequence_numbers_in_flight() : 0;

  // Create and store new TCPSenderMessage objects for each segment of data to be sent.
  while ( actual_window && !fin_sent ) {
    TCPSenderMessage msg;
    uint64_t seg_size = min( actual_window, min( TCPConfig::MAX_PAYLOAD_SIZE, outbound_stream.peek().length() ) );

    if ( !syn_set ) {
      msg.SYN = true;
      syn_set = true;
    }

    msg.payload = Buffer( std::string { outbound_stream.peek().data(), seg_size } );
    outbound_stream.pop( seg_size );
    actual_window -= seg_size;
    msg.seqno = isn_;

    // check space for FIN flag
    if ( outbound_stream.is_finished() && actual_window ) {
      msg.FIN = true;
      fin_sent = true;
    }

    if ( msg.sequence_length() == 0 )
      break;
    outstanding_segs.push_back( msg );
    sent_segs.push_back( msg );
    isn_ = isn_ + msg.sequence_length();
  }
}

// Creates and returns an empty TCPSenderMessage with the current ISN.
TCPSenderMessage TCPSender::send_empty_message() const
{
  TCPSenderMessage empty;
  empty.seqno = isn_;
  return empty;
}

// Handles received acknowledgments and updates the sender's state.
void TCPSender::receive( const TCPReceiverMessage& msg )
{
  window = msg.window_size;
  bool acked = false;

  if ( !msg.ackno || ( msg.ackno.value().unwrap( zero_point, 0 ) > isn_.unwrap( zero_point, 0 ) ) )
    return;

  // Remove sent but acknowledged segments from the sent_segs list.
  while ( !sent_segs.empty()
          && sent_segs.front().seqno.unwrap( zero_point, 0 ) + sent_segs.front().sequence_length()
               <= msg.ackno.value().unwrap( zero_point, 0 ) ) {
    sent_segs.pop_front(); // acknowledged
    acked = true;
  }

  // If a new acknowledgment was received, reset the RTO timer and consecutive retransmissions counter.
  if ( acked ) {
    elapsed_time = 0;
    retransmissions = 0;
    alarm = initial_RTO_ms_;
    if ( !outstanding_segs.empty() ) {
      outstanding_segs.pop_front();
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick )
{
  elapsed_time += ms_since_last_tick;

  // If the RTO timer has reached the alarm threshold, handle timeouts and retransmissions.
  if ( elapsed_time >= alarm ) {
    // If there are unacknowledged segments and the window size permits, retransmit the oldest unacknowledged
    // segment.
    if ( window > 0 ) {
      retransmissions++;
      alarm *= 2;
    }
    if ( !sent_segs.empty() ) {
      outstanding_segs.push_back( sent_segs.front() );
    }
    elapsed_time = 0;
  }
}
