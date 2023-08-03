#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <deque>

class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

protected:
  std::deque<TCPSenderMessage> outstanding_segs {}; // Deque to store unacknowledged TCP segments
  std::deque<TCPSenderMessage> sent_segs {};        // Deque to store sent TCP segments
  uint64_t window { 1 };                            // Sender's window size
  uint64_t retransmissions { 0 };                   // Counter for consecutive retransmissions
  size_t elapsed_time { 0 };                        // Elapsed time since the last RTO event
  bool syn_set { false };                           // Flag to indicate whether the SYN has been set
  size_t alarm = initial_RTO_ms_;                   // Alarm threshold for the retransmission timer
  Wrap32 zero_point = isn_;                         // Reference point for sequence number unwrapping
  bool fin_sent { false };                          // Flag to indicate whether the FIN has been sent

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
