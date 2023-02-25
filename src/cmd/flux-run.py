##############################################################
# Copyright 2023 Lawrence Livermore National Security, LLC
# (c.f. AUTHORS, NOTICE.LLNS, COPYING)
#
# This file is part of the Flux resource manager framework.
# For details, see https://github.com/flux-framework.
#
# SPDX-License-Identifier: LGPL-3.0
##############################################################

import logging
import sys

import flux
import flux.cli.run as base

LOGGER = logging.getLogger("flux-run")


@flux.util.CLIMain(LOGGER)
def main():
    sys.stdout = open(
        sys.stdout.fileno(), "w", encoding="utf8", errors="surrogateescape"
    )
    sys.stderr = open(
        sys.stderr.fileno(), "w", encoding="utf8", errors="surrogateescape"
    )

    # Prepare the submit parser
    run = base.RunCmd(
        description="run a job interactively",
    )
    parser = run.get_parser()
    parser.set_defaults(func=run.main)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()

# vi: ts=4 sw=4 expandtab
