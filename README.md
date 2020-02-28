# tcp_echo_server

Function:
Simple TCP echo server to send text information from a command prompt across a network socket, packed using msgpack, and then repeat the information back to the client.  

Dependencies: msgpack
Clone the https://github.com/msgpack/msgpack-c repo and follow build instructions for msgpack.

Building:
Run make all to build the server and client binaries.  Run make clean to clean up the object files.

Usage:
  server: listens on all interfaces on a specified port
  client: specify IP and port to connect to


