FROM ubuntu:20.04

WORKDIR /usr/src/app

COPY . .

#install msgpack-c dependecies
RUN apt-get update
RUN apt-get install gcc cmake git -y
RUN git clone https://github.com/msgpack/msgpack-c.git
CMD cd msgpack-c
CMD cmake .
CMD make
CMD make install

#install the server
CMD cd /usr/src/app
CMD make all

# open the port and start the server
EXPOSE 1234
CMD ./server -p 1234
