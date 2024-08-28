// byte_stream.cc
// This file implements a ByteStream class that provides a simple,
// thread-safe mechanism for buffering and transferring data between
// a writer and a reader with a specified capacity.

#include "byte_stream.hh"

using namespace std;

// ByteStream constructor
// Initializes a ByteStream with a specified capacity.
ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), bytes_pushed_( 0 ), bytes_popped_( 0 ), closed_( false ), error_( false ), buffer_( "" )
{}

// Pushes data to the buffer.
// If the ByteStream is not closed or in error state, appends data to the buffer.
void Writer::push( string data )
{
  if ( !closed_ && !error_ ) {
    uint64_t free_space = available_capacity();
    uint64_t to_push = min( free_space, data.size() );
    buffer_.append( data, 0, to_push );
    bytes_pushed_ += to_push;
  }
}

// Closes the ByteStream for writing.
void Writer::close()
{
  closed_ = true;
}

// Sets the error state for the ByteStream.
void Writer::set_error()
{
  error_ = true;
}

// Returns whether the ByteStream is closed for writing.
bool Writer::is_closed() const
{
  return closed_;
}

// Returns the available capacity in the buffer.
uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

// Returns the total number of bytes pushed to the buffer.
uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

// Returns a string_view of the buffer.
string_view Reader::peek() const
{
  return string_view( buffer_ );
}

// Returns whether the ByteStream has been closed and the buffer is empty.
bool Reader::is_finished() const
{
  return closed_ && buffer_.empty();
}

// Returns whether the ByteStream has encountered an error.
bool Reader::has_error() const
{
  return error_;
}

// Removes a specified number of bytes from the buffer.
void Reader::pop( uint64_t len )
{
  uint64_t to_pop = min( len, buffer_.size() );
  buffer_.erase( 0, to_pop );
  bytes_popped_ += to_pop;
}

// Returns the current size of the buffer.
uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}

// Returns the total number of bytes popped from the buffer.
uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
