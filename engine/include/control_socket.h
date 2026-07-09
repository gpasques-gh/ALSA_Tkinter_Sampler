#ifndef CONTROL_SOCKET_H
#define CONTROL_SOCKET_H

#include "sampler.h"

int control_socket_open(const char *path);

void control_socket_accept(int listen_fd, int *client_fd);

void control_socket_poll(int *client_fd, sampler_t *sampler);

#endif