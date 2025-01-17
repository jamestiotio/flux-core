.. flux-help-section: jobs

============
flux-jobs(1)
============


SYNOPSIS
========

**flux** **watch** [*OPTIONS*] [JOBID ...]

DESCRIPTION
===========

The flux-watch(1) command is used to monitor the output and state of one
or more Flux jobs. The command works similarly to the :man1:`flux-submit`
``--watch`` option, but can be used to monitor even inactive jobs. For
example, to copy all job output to the terminal after submitting a series
of jobs with :man1:`flux-submit` or :man1:`flux-bulksubmit`, use

::

  flux watch --all

This command can also be used at the end of a batch script to wait for all
submitted jobs to complete and copy all output to the same location as the
batch job.

OPTIONS
=======

**-a, --active**
   Watch all active jobs.
   This is equivalent to  *--filter=pending,running*.

**-A, --all**
   Watch all jobs. This is equivalent to *--filter=pending,running,inactive*.

**-c, --count**\ *=N*
   Limit output to N jobs (default 1000). This is a safety measure to
   protect against watching too many jobs with the ``--all`` option. The
   limit can be disabled with ``--count=0``.

**--since**\ *=WHEN*
   Limit output to jobs that have been active since a given timestamp.
   This option implies ``-a`` if no other ``--filter`` options are specified.
   If *WHEN* begins with ``-`` character, then the remainder is considered
   to be a an offset in Flux standard duration (RFC 23). Otherwise,
   any datetime expression accepted by the Python `parsedatetime
   <https://github.com/bear/parsedatetime>`_ module is accepted. Examples:
   "-6h", "-1d", "yesterday", "2021-06-21 6am", "last Monday", etc. It is
   assumed to be an error if a timestamp in the future is supplied.

**-f, --filter**\ *=STATE|RESULT*
   Watch jobs with specific job state or result. Multiple states or results
   can be listed separated by comma. See the JOB STATUS section in the
   :man1:`flux-jobs` manual for additional information.

**--progress**
   Display a progress bar showing the completion progress of monitored
   jobs.  Jobs that are already inactive will immediately have their
   progress updated in the progress bar, with output later copied to the
   terminal. The progress bar by default includes a count of pending,
   running, complete and failed jobs, and an elapsed timer. The elapsed
   timer is initialized at the submit time of the earliest job, or the
   starttime of the instance with ``--all``, in order to reflect the real
   elapsed time for the jobs being monitored.

**--jps**
   With ``--progress``, display throughput statistics (job/s) in the
   progress bar instead of an elapsed timer. Note: The throughput will be
   calculated based on the elapsed time as described in the description
   of the ``-progress`` option.

EXIT STATUS
===========

The exit status of ``flux watch`` is 0 if no jobs match the job selection
options or if all jobs complete with success. Otherwise, the command exits
with the largest exit status of all monitored jobs, or 2 if there is an
error during option processing.

RESOURCES
=========

Flux: http://flux-framework.org

SEE ALSO
========

:man1:`flux-jobs`, :man1:`flux-submit`, :man1:`flux-bulksubmit`
