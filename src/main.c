/*
* Copyright (c) 2023 Nordic Semiconductor
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_echo_client_svc_sample, LOG_LEVEL_DBG);

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/net/socket_service.h>

#define MY_PORT 2111

K_MUTEX_DEFINE(lock);
static struct pollfd sockfd_tcp_client;

static void receive_data(struct net_socket_service_event *pev,
      char *buf, size_t buflen);
static void restart_echo_service(void);

static void tcp_service_handler(struct net_socket_service_event *pev)
{
  if (pev->event.revents & POLLIN)
  {
    static char buf[1500];

    receive_data(pev, buf, sizeof(buf));
  }

  if (pev->event.revents & POLLOUT)
  {
    LOG_INF("Socket %d connected", pev->event.fd);
  }
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_tcp, tcp_service_handler, 1);

static void tcp_client_set(int sock)
{
  int ret;

  k_mutex_lock(&lock, K_FOREVER);

  sockfd_tcp_client.fd = sock;
  sockfd_tcp_client.events = POLLIN | POLLOUT;

  ret = net_socket_service_register(&service_tcp, &sockfd_tcp_client, 1, NULL);

  if (ret < 0)
  {
    LOG_ERR("Cannot register socket service handler (%d). "
      "Attempting to restart service.", ret);
    restart_echo_service();
  }

  k_mutex_unlock(&lock);
}

static void tcp_client_remove(void)
{
  int ret;

  k_mutex_lock(&lock, K_FOREVER);

  close(sockfd_tcp_client.fd);
  sockfd_tcp_client.fd = -1;
  ret = net_socket_service_register(&service_tcp, &sockfd_tcp_client, 1, NULL);

  if (ret < 0)
  {
    LOG_ERR("Cannot register socket service handler (%d). "
      "Attempting to restart service.", ret);
    restart_echo_service();
  }
  
  k_mutex_unlock(&lock);
}

static void receive_data(struct net_socket_service_event *pev,
      char *buf, size_t buflen)
{
  struct pollfd *pfd = &pev->event;
  int client = pfd->fd;
  struct sockaddr_in addr;
  char addr_str[INET_ADDRSTRLEN];
  socklen_t addrlen = sizeof(addr);
  int len, out_len;
  char *p;

  len = recvfrom(client, buf, buflen, 0,
          (struct sockaddr *)&addr, &addrlen);
  if (len <= 0) {
    if (len < 0) {
      LOG_ERR("recv: %d", -errno);
    }
    tcp_client_remove();
    inet_ntop(addr.sin_family, &addr.sin_addr, addr_str, sizeof(addr_str));
    LOG_INF("Connection from %s closed", addr_str);
    return;
  }

  p = buf;
  do {
    out_len = send(client, p, len, 0);

    if (out_len < 0) {
      LOG_ERR("sendto: %d", -errno);
      break;
    }

    p += out_len;
    len -= out_len;
  } while (len);
}

static int start_echo_service(void)
{
  int tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (tcp_sock < 0) {
    LOG_ERR("socket: %d", -errno);
    return -errno;
  }

  /* Set socket to non-blocking mode */
  zsock_fcntl(tcp_sock, ZVFS_F_SETFL, 
              zsock_fcntl(tcp_sock, ZVFS_F_GETFL, 0) | ZVFS_O_NONBLOCK);

  tcp_client_set(tcp_sock);

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(MY_PORT),
  };

  net_addr_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &addr.sin_addr);
  
  connect(tcp_sock, (struct sockaddr*)&addr, sizeof(addr));

  return 0;
}

static int stop_echo_service(void)
{
  (void)net_socket_service_unregister(&service_tcp);

  k_mutex_lock(&lock, K_FOREVER);

  if (sockfd_tcp_client.fd != -1)
  {
    close(sockfd_tcp_client.fd);
    sockfd_tcp_client.fd = -1;
  }

  k_mutex_unlock(&lock);

  return 0;
}

static void restart_echo_service(void)
{
  stop_echo_service();
  start_echo_service();
}

SYS_INIT(start_echo_service, APPLICATION, 99);
