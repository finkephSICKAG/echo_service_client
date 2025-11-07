Overview
********

The sockets/echo_service_client sample application for Zephyr implements a TCP
echo client supporting IPv4 and using a BSD Sockets compatible API.

The purpose of this sample is to show how to use socket service API.
The socket service is a concept where many blocking sockets can be listened by
one thread, and which can then trigger a callback if there is activity in the set
of sockets. This saves memory as only one thread needs to be created in the
system.

The application supports IPv4 and TCP.

Requirements
************

- A board with hardware networking.

Building and Running
********************

Build the Zephyr version of the sockets/echo_service_client application like this:

.. code-block:: console

    west build -b <board_to_use> samples/net/sockets/echo_service_client

After the sample starts, it tries to connect to a server running at 192.168.0.12 
and port 2111.

The easiest way to have a server of this kind is to use the provided TCP server script:

.. code-block:: console

    $ python ./scripts/test_tcp_server.py

After a connection is made, the application will echo back any line sent to it by the server.