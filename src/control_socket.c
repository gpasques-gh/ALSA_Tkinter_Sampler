#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "control_socket.h"

static void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int control_socket_open(const char *path)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    unlink(path); /* remove stale socket file from a previous run */

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 1) < 0) 
    {
        perror("listen");
        close(fd);
        return -1;
    }

    set_nonblocking(fd); /* accept() must never stall the audio loop */
    return fd;
}

void control_socket_accept(int listen_fd, int *client_fd)
{
    if (*client_fd >= 0) return; /* already have a client */

    int fd = accept(listen_fd, NULL, NULL);
    if (fd < 0) 
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("accept");
        return;
    }

    set_nonblocking(fd);
    *client_fd = fd;
    fprintf(stderr, "control socket: GUI connected\n");
}

/* Parses one line like "TRIGGER 0 100", "SET_PITCH 2 1.5", or "STATUS"
 * and applies it to the sampler. `reply` (may be empty) receives an
 * optional response to write back to the client. */
static void handle_gui_command(char *line, sampler_t *sampler,
                                char *reply, size_t reply_len)
{
    char verb[32] = {0};
    int idx = -1;

    if (sscanf(line, "%31s", verb) != 1) return;

    if (strcmp(verb, "TRIGGER") == 0)
    {
        int velocity = 127;
        if (sscanf(line, "%*s %d %d", &idx, &velocity) < 1) return;
        if (idx < 0 || idx >= sampler->sample_count) return;

        sample_t *s = sampler->samples[idx];
        s->snd_buffer_pos = 0.0f;
        s->velocity = (uint8_t) velocity;
        s->active = 1;
    } 
    else if (strcmp(verb, "SET_PITCH") == 0) 
    {
        float fval = 1.0f;
        if (sscanf(line, "%*s %d %f", &idx, &fval) != 2) return;
        if (idx < 0 || idx >= sampler->sample_count) return;
        sampler->samples[idx]->pitch += fval;
    } 
    else if (strcmp(verb, "STATUS") == 0) 
    {
        size_t off = 0;
        for (uint8_t i = 0; i < sampler->sample_count && off < reply_len; i++) {
            sample_t *s = sampler->samples[i];
            off += snprintf(reply + off, reply_len - off,
                             "%s:%d ", s->sample_name, s->active);
        }
        if (off < reply_len) reply[off++] = '\n';
    }
    else if (strcmp(verb, "ADD_SAMPLE") == 0)
    {
        if (sampler->sample_count >= SAMPLES) return;
        char file_name[256];
        char sample_name[256];
        if (sscanf(line, "%*s %255s %255s", &sample_name, &file_name) != 2) return;
        uint8_t res = init_sample(sampler->samples[sampler->sample_count], sample_name, file_name);
        if (!res) sampler->sample_count++;
    }
    /* unrecognized verbs are silently ignored */
}

void control_socket_poll(int *client_fd, sampler_t *sampler)
{
    if (*client_fd < 0) return;

    char buf[256];
    ssize_t n = read(*client_fd, buf, sizeof(buf) - 1);

    if (n <= 0) {
        if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            close(*client_fd);
            *client_fd = -1;
            fprintf(stderr, "control socket: GUI disconnected\n");
            exit(EXIT_SUCCESS);
        }
        return;
    }
    buf[n] = 0;

    /* a single read() can contain multiple newline-separated commands */
    char reply[256];
    char *line = strtok(buf, "\n");
    while (line) {
        reply[0] = 0;
        handle_gui_command(line, sampler, reply, sizeof(reply));
        if (reply[0]) write(*client_fd, reply, strlen(reply));
        line = strtok(NULL, "\n");
    }
}