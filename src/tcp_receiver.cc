#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <cstdlib>
using namespace std;

// This method processes incoming messages from the TCPSender and forwards the payload to the Reassembler.
void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // If the SYN flag is set, initialize the Initial Sequence Number (ISN) and set 'zero' to the sequence number of
  // the message.
  if ( message.SYN ) {
    isn = true;
    zero = message.seqno;
  }

  // If the ISN has not been set yet, discard the message.
  if ( !isn ) {
    return;
  }

  // Calculate the checkpoint by subtracting the ISN from the bytes pushed into the inbound stream.
  uint64_t checkpoint = inbound_stream.bytes_pushed() - isn;

  // Calculate the absolute sequence number using unwrap().
  uint64_t abs_seqno = message.seqno.unwrap( zero, checkpoint );

  // Adjust the sequence number if the SYN flag is not set.
  if ( !message.SYN ) {
    abs_seqno -= 1;
  }

  // Insert the payload into the Reassembler, along with the absolute sequence number and the FIN flag.
  reassembler.insert( abs_seqno, message.payload, message.FIN, inbound_stream );
}

// This method creates and returns a TCPReceiverMessage containing the ACK number and the window size.
TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  TCPReceiverMessage message;

  // If the TCPReceiver has received the Initial Sequence Number (ISN),
  // set the ACK number to the next sequence number needed by the TCPReceiver.
  if ( isn ) {
    Wrap32 ackno_value = zero + isn + inbound_stream.bytes_pushed() + inbound_stream.is_closed();
    message.ackno = std::optional<Wrap32> { ackno_value };
  }

  // Calculate the available window size, taking the minimum of the available capacity and UINT16_MAX.
  message.window_size = min( static_cast<int>( inbound_stream.available_capacity() ), UINT16_MAX );

  return message;
}
