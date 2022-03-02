/************************************************************\
 * Copyright 2022 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <jansson.h>
#include <flux/core.h>
#include <time.h>

#include "kvs_checkpoint.h"

flux_future_t *kvs_checkpoint_commit (flux_t *h,
                                      const char *key,
                                      const char *rootref)
{
    flux_future_t *f = NULL;
    double timestamp;

    if (!h || !key || !rootref) {
        errno = EINVAL;
        return NULL;
    }

    timestamp = flux_reactor_now (flux_get_reactor (h));

    if (!(f = flux_rpc_pack (h,
                             "kvs-checkpoint.put",
                             0,
                             0,
                             "{s:s s:{s:i s:s s:f}}",
                             "key",
                             key,
                             "value",
                             "version", 1,
                             "rootref", rootref,
                             "timestamp", timestamp)))
        return NULL;

    return f;
}

flux_future_t *kvs_checkpoint_lookup (flux_t *h, const char *key)
{
    if (!h || !key) {
        errno = EINVAL;
        return NULL;
    }

    return flux_rpc_pack (h,
                          "kvs-checkpoint.get",
                          0,
                          0,
                          "{s:s}",
                          "key",
                          key);
}

int kvs_checkpoint_lookup_get_rootref (flux_future_t *f, const char **rootref)
{
    const char *tmp_rootref;
    int version;

    if (!f || !rootref) {
        errno = EINVAL;
        return -1;
    }

    if (flux_rpc_get_unpack (f, "{s:{s:i s:s}}",
                                "value",
                                "version", &version,
                                "rootref", &tmp_rootref) < 0)
        return -1;

    if (version != 0 && version != 1) {
        errno = EINVAL;
        return -1;
    }

    (*rootref) = tmp_rootref;
    return 0;
}

/* returns "N/A" if not available */
int kvs_checkpoint_lookup_get_formatted_timestamp (flux_future_t *f,
                                                   char *buf,
                                                   size_t len)
{
    int version;
    double timestamp = 0.;

    if (!f || !buf) {
        errno = EINVAL;
        return -1;
    }

    if (flux_rpc_get_unpack (f, "{s:{s:i s?f}}",
                                "value",
                                "version", &version,
                                "timestamp", &timestamp) < 0)
        return -1;

    if (version != 0 && version != 1) {
        errno = EINVAL;
        return -1;
    }

    if (version == 1) {
        time_t sec = timestamp;
        struct tm tm;
        gmtime_r (&sec, &tm);
        if (strftime (buf, len, "%FT%T", &tm) == 0) {
            errno = EINVAL;
            return -1;
        }
    }
    else { /* version == 0 */
        if (snprintf (buf, len, "N/A") >= len) {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */