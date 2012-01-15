/*
 * netrelay.c: Listen data from a client and a server and send it to
 * each other
 *
 * Copyright 2012 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <assert.h>         /* assert */
#include <errno.h>          /* errno */
#include <fcntl.h>          /* open, fcntl */
#include <string.h>         /* strerror */
#include <stdio.h>          /* *printf */
#include <stdint.h>         /* std data types */
#include <stdlib.h>         /* exit */
#include <unistd.h>         /* close, read, fcntl */
#include <arpa/inet.h>      /* inet_ntoa, htons */
#include <netinet/in.h>     /* INADDR_ANY, inet_ntoa */
#include <sys/select.h>     /* select */
#include <sys/socket.h>     /* socket, bind, listen,
                             * inet_ntoa, setsockopt, recvfrom */
#include <sys/stat.h>       /* open */
#include <sys/types.h>      /* recvfrom */


#define NETRELAY_CLIENT_STREAM_PORT     8500
#define NETRELAY_SERVER_STREAM_PORT     12347

int open_udp_socket(int port);

int
main(int argc, char **argv)
{
  int client_stream_listen_fd = -1, server_stream_listen_fd = -1;
  int client_data_fd = -1;
  uint16_t server_stream_port = 0;
  uint16_t client_stream_port = 0;
  struct sockaddr_in si_server;
  int slen_server = sizeof(si_server);
  struct sockaddr_in si_client;
  int slen_client = sizeof(si_client);

  /* Open listening socket for client stream connection */
  client_stream_listen_fd = open_udp_socket(NETRELAY_CLIENT_STREAM_PORT);
  if (client_stream_listen_fd == -1) {
	fprintf(stderr,
			"Failed to create client stream listen socket for port %d.\n",
			NETRELAY_CLIENT_STREAM_PORT);
	exit(-1);
  }

  /* Open listening socket for server stream connection */
  server_stream_listen_fd = open_udp_socket(NETRELAY_SERVER_STREAM_PORT);
  if (server_stream_listen_fd == -1) {
	fprintf(stderr,
			"Failed to create server stream listen socket for port %d.\n",
			NETRELAY_SERVER_STREAM_PORT);
	close(client_stream_listen_fd);
	exit(-1);
  }

  /* Listen for new data */
  while (1) {
    fd_set fds;
    int retval;
	int max_fd = client_stream_listen_fd;
	struct in_addr  server_stream_addr;
	struct in_addr  client_stream_addr;

    FD_ZERO(&fds);

	/* Add client stream fd for udp packets */
    FD_SET(client_stream_listen_fd, &fds);

	/* Add server stream fd for udp packets */
    FD_SET(server_stream_listen_fd, &fds);
	if (server_stream_listen_fd > max_fd) {
	  max_fd = server_stream_listen_fd;
	}

	/* Add client data fd for new data, if connected */
	if (client_data_fd > -1) {
	  FD_SET(client_data_fd, &fds);
	  if (client_data_fd > max_fd) {
		max_fd = client_data_fd;
	  }
	}

	retval = select(max_fd + 1, &fds, NULL, NULL, NULL);
    if(retval == -1) {
	  fprintf(stderr, "Failed to select: %s\n",
			  strerror(errno));
	  exit(-1);
    }

	if(retval == 0) {
	  printf( "Select timeout (shouldn't happen)\n");
	  continue;
	}

	/*printf("New data in %d sockets\n", retval);*/

	/* Check which socket has new data */

	/* New client stream data */
	if (FD_ISSET(client_stream_listen_fd, &fds)) {
	  char buf[4096];
	  ssize_t bytes_recv;

	  printf("data from client_stream_listen_fd\n");

	  /* Get data from client */
	  bytes_recv = recvfrom(client_stream_listen_fd, buf, 4096, 0,
							(struct sockaddr *)&si_client, &slen_client);
	  if (bytes_recv == -1) {
		fprintf(stderr, "Failed to receive UDP data from client: %s\n",
				strerror(errno));
	  } else {

		client_stream_addr = si_client.sin_addr;
		client_stream_port = si_client.sin_port;

		printf("Received server packet from %s:%d\n",
			   inet_ntoa(client_stream_addr), ntohs(client_stream_port));

		printf("Server port: %d\n", ntohs(server_stream_port));

		if (ntohs(server_stream_port) > 0) {
		  ssize_t bytes_sent;

		  printf("Sending client data to %s:%d\n",
				 inet_ntoa(server_stream_addr), ntohs(server_stream_port));

		  /* Send data to server */
		  bytes_sent = sendto(server_stream_listen_fd,
							  buf, (size_t)bytes_recv, 0,
							  (struct sockaddr *)&si_server, slen_server);

		  if (bytes_sent < 0) {
			fprintf(stderr, "Failed to send UDP data to server: %s\n",
					strerror(errno));
		  }

		  if (bytes_sent < bytes_recv) {
			fprintf(stderr, "Failed to send all UDP data to server: %d < %d\n",
					bytes_sent, bytes_recv);
		  }

		}
	  }

	}

	/* New server stream data */
	if (FD_ISSET(server_stream_listen_fd, &fds)) {
	  char buf[4096];
	  ssize_t bytes_recv;

	  printf("data from server_stream_listen_fd\n");

	  /* Get data from server and send it to client */
	  bytes_recv = recvfrom(server_stream_listen_fd,
							buf, 4096, 0,
							(struct sockaddr *)&si_server, &slen_server);
	  if ( bytes_recv == -1) {
		fprintf(stderr, "Failed to receive UDP data from server: %s\n", strerror(errno));
	  } else {
		ssize_t bytes_sent;

		server_stream_addr = si_server.sin_addr;
		server_stream_port = si_server.sin_port;

		printf("Received server packet from %s:%d\n",
			   inet_ntoa(server_stream_addr), ntohs(server_stream_port));

		  /* Send data to client, if we have the address */
		if (ntohs(client_stream_port) > 0) {
		  bytes_sent = sendto(client_stream_listen_fd,
							  buf, (size_t)bytes_recv, 0,
							  (struct sockaddr *)&si_client, slen_client);
		  
		  if (bytes_sent < 0) {
			fprintf(stderr, "Failed to send UDP data to client: %s\n",
					strerror(errno));
		  }
		  
		  if (bytes_sent < bytes_recv) {
			fprintf(stderr, "Failed to send all UDP data to client: %d < %d\n",
					bytes_sent, bytes_recv);
		  }
		} else {
		  fprintf(stderr, "No client port, not sending data to client\n");
		}
	  }
	}
  }

  return 0;
}


/*
 * Open UDP listening socket
 */
int open_udp_socket(int port)
{

  int fd;
  struct sockaddr_in addr;
  int opt = 1;

  /* try to create a socket */
  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd == -1) {
	fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
	return -1;
  }

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) < 0) {
	fprintf(stderr, "Failed to set SO_REUSEADDR: %s\n",
			strerror(errno));
	return -1;
  }

  /* Clear structure */
  memset(&addr, 0, sizeof(struct sockaddr));

  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(fd,
		   (struct sockaddr *) &addr,
		   sizeof(struct sockaddr_in)) == -1) {
	fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
	close(fd);
	return 0;
  }

  return fd;
}

