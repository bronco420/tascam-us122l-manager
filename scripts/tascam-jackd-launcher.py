#!/usr/bin/env python3
"""Subreaper launcher per jackd.

Avvia jackd come figlio diretto e resta vivo finche' jackd vive.
Imposta PR_SET_CHILD_SUBREAPER: quando jackd muore, il launcher lo
reap immediatamente via waitpid, evitando la formazione di zombie
sotto systemd --user (che non li raccoglie mai).

Uso:
    tascam-jackd-launcher.py --pidfile FILE -- jackd [args...]

Scrive il PID del jackd reale in FILE.
"""

import os
import signal
import subprocess
import sys

try:
    import ctypes
    PR_SET_CHILD_SUBREAPER = 36
    libc = ctypes.CDLL(None, use_errno=True)
    libc.prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0)
except Exception:
    pass


def parse_args(argv):
    pidfile = None
    rest = []
    i = 0
    while i < len(argv):
        if argv[i] == "--pidfile" and i + 1 < len(argv):
            pidfile = argv[i + 1]
            i += 2
        elif argv[i] == "--":
            rest = argv[i + 1:]
            break
        else:
            rest.append(argv[i])
            i += 1
    return pidfile, rest


def main():
    pidfile, cmd = parse_args(sys.argv[1:])
    if not cmd:
        sys.stderr.write("launcher: nessun comando\n")
        return 2

    try:
        # Disable the D-Bus audio reservation protocol: on many systems there is
        # no provider for "Audio0" (org.freedesktop.ReserveDevice1) and jackd
        # fails to acquire the device otherwise.
        child_env = dict(os.environ)
        child_env.setdefault("JACK_NO_AUDIO_RESERVATION", "1")
        # Detach into a new session so jackd survives after the caller
        # process (CLI/GUI) exits.
        proc = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, env=child_env,
                                start_new_session=True)
    except FileNotFoundError:
        sys.stderr.write("launcher: comando non trovato: %s\n" % cmd[0])
        return 127

    if pidfile:
        try:
            with open(pidfile, "w") as f:
                f.write(str(proc.pid))
        except OSError:
            pass

    def forward(signum, frame):
        try:
            proc.send_signal(signum)
        except OSError:
            pass

    for s in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP, signal.SIGQUIT):
        signal.signal(s, forward)

    try:
        while True:
            try:
                rc = proc.wait()
                break
            except KeyboardInterrupt:
                continue
    finally:
        try:
            os.waitpid(proc.pid, 0)
        except OSError:
            pass

    return rc if rc is not None else 0


if __name__ == "__main__":
    sys.exit(main())
