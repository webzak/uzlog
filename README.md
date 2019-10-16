
# UDP zero log receiver

The simple logging/debugging tool receiving messages over UDP. The purpose is to have minimal impact on debugged application.


## Usage

uzlog [options]

**-h** help

**-p port** port to listen

**-a ip** ip address

**-o** output to stdout

**-c** allow colored output

**-f** allow formatted output

**-w dir** write sessions to files


Typical usage: **./uzlog -fco -p 7777**


## Protocol

The messages are sent only one way via network to endpoint. Each message belongs to specific session. The protocol is unreliable, if some packets are dropped by network, the message will be skipped.

### Transport layer

Byte order - big endian

#### Message structure:

| message |  CRC 32 |

#### Packet structure

Max packet size  508 bytes

| session (8b) | message id (4b) | message length (4b) | message offset (4b) | payload  (max 488b) |

### Message layer

First byte determines the message type and formatting for the rest of the message:

|message type (1b) | payload |

#### Message types:

1 - LOG

#### Log message format

| message type = 1 (1b) | fg color (1b) | bg color (1b) |  0 (1b) | payload |
