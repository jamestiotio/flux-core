/************************************************************\
 * Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/* Note:
 * This connector creates a 0MQ inproc socket that communicates with another
 * inproc socket in the same process (normally the flux broker).  Pairs of
 * inproc sockets must share a common 0MQ context. The context is passed
 * in as a URI query option, e.g. "shmem://NAME&zctx=%p", where NAME is
 * the unique socket name used to match two endpoints.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <errno.h>
#include <zmq.h>
#include <argz.h>
#if HAVE_CALIPER
#include <caliper/cali.h>
#endif
#include <flux/core.h>

#include "src/common/libzmqutil/msg_zsock.h"
#include "src/common/libzmqutil/sockopt.h"
#include "ccan/str/str.h"

typedef struct {
    void *zctx;
    void *sock;
    char *uuid;
    flux_t *h;
    char *argz;
    size_t argz_len;
    char endpoint[128];
} shmem_ctx_t;

static const struct flux_handle_ops handle_ops;

static int op_pollevents (void *impl)
{
    shmem_ctx_t *ctx = impl;
    int e;
    int revents = 0;

    if (zgetsockopt_int (ctx->sock, ZMQ_EVENTS, &e) < 0)
        return -1;
    if (e & ZMQ_POLLIN)
        revents |= FLUX_POLLIN;
    if (e & ZMQ_POLLOUT)
        revents |= FLUX_POLLOUT;
    if (e & ZMQ_POLLERR)
        revents |= FLUX_POLLERR;

    return revents;
}

static int op_pollfd (void *impl)
{
    shmem_ctx_t *ctx = impl;
    int fd;

    if (zgetsockopt_int (ctx->sock, ZMQ_FD, &fd) < 0)
        return -1;
    return fd;
}

static int op_send (void *impl, const flux_msg_t *msg, int flags)
{
    shmem_ctx_t *ctx = impl;

    return zmqutil_msg_send (ctx->sock, msg);
}

static flux_msg_t *op_recv (void *impl, int flags)
{
    shmem_ctx_t *ctx = impl;
    zmq_pollitem_t zp = {
        .events = ZMQ_POLLIN,
        .socket = ctx->sock,
        .revents = 0,
        .fd = -1,
    };
    flux_msg_t *msg = NULL;

    if ((flags & FLUX_O_NONBLOCK)) {
        int n;
        if ((n = zmq_poll (&zp, 1, 0L)) <= 0) {
            if (n == 0)
                errno = EWOULDBLOCK;
            goto done;
        }
    }
    msg = zmqutil_msg_recv (ctx->sock);
done:
    return msg;
}

static void op_fini (void *impl)
{
    shmem_ctx_t *ctx = impl;

    (void)zmq_close (ctx->sock);
    free (ctx->argz);
    free (ctx);
}

flux_t *connector_init (const char *path, int flags, flux_error_t *errp)
{
#if HAVE_CALIPER
    cali_id_t uuid   = cali_create_attribute ("flux.uuid",
                                              CALI_TYPE_STRING,
                                              CALI_ATTR_SKIP_EVENTS);
    size_t length = strlen(path);
    cali_push_snapshot ( CALI_SCOPE_PROCESS | CALI_SCOPE_THREAD,
                         1, &uuid, (const void **)&path, &length);
#endif

    shmem_ctx_t *ctx = NULL;
    char *item;
    int e;
    int bind_socket = 0; // if set, call bind on socket, else connect

    if (!path) {
        errno = EINVAL;
        goto error;
    }
    if (!(ctx = calloc (1, sizeof (*ctx)))) {
        errno = ENOMEM;
        goto error;
    }
    if ((e = argz_create_sep (path, '&', &ctx->argz, &ctx->argz_len)) != 0) {
        errno = e;
        goto error;
    }
    ctx->uuid = item = argz_next (ctx->argz, ctx->argz_len, NULL);
    if (!ctx->uuid) {
        errno = EINVAL;
        goto error;
    }
    while ((item = argz_next (ctx->argz, ctx->argz_len, item))) {
        if (streq (item, "bind"))
            bind_socket = 1;
        else if (streq (item, "connect"))
            bind_socket = 0;
        else if (strstarts (item, "zctx=")) {
            if (sscanf (item + 5, "%p", &ctx->zctx) != 1) {
                errno = EINVAL;
                goto error;
            }
        }
        else {
            errno = EINVAL;
            goto error;
        }
    }
    if (!(ctx->sock = zmq_socket (ctx->zctx, ZMQ_PAIR))
        || zsetsockopt_int (ctx->sock, ZMQ_LINGER, 5) < 0
        || zsetsockopt_int (ctx->sock, ZMQ_SNDHWM, 0) < 0
        || zsetsockopt_int (ctx->sock, ZMQ_RCVHWM, 0) < 0)
        goto error;
    snprintf (ctx->endpoint, sizeof (ctx->endpoint), "inproc://%s", ctx->uuid);
    if (bind_socket) {
        if (zmq_bind (ctx->sock, ctx->endpoint) < 0)
            goto error;
    }
    else {
        if (zmq_connect (ctx->sock, ctx->endpoint) < 0)
            goto error;
    }
    if (!(ctx->h = flux_handle_create (ctx, &handle_ops, flags)))
        goto error;
    return ctx->h;
error:
    if (ctx) {
        int saved_errno = errno;
        op_fini (ctx);
        errno = saved_errno;
    }
    return NULL;
}

static const struct flux_handle_ops handle_ops = {
    .pollfd = op_pollfd,
    .pollevents = op_pollevents,
    .send = op_send,
    .recv = op_recv,
    .getopt = NULL,
    .setopt = NULL,
    .impl_destroy = op_fini,
};

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
