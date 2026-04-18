"""
Serializes GCC subprocess spawning to work around Windows MSYS2 / xtensa toolchain
limitation: concurrent xtensa-esp32s3-elf-g++ instances fail with
"CreateProcess: No such file or directory" when they simultaneously try to
fork/exec cc1plus.

Strategy: wrap env['SPAWN'] with a threading.Semaphore(1) so only ONE
compiler process runs at a time, regardless of how many parallel SCons jobs
PlatformIO configured (-j N).  The semaphore is stored in sys.modules so it
is shared across every SCons environment created in the same Python process.
"""

Import("env")
import sys
import threading

# ── Shared semaphore (one slot = serial) ────────────────────────────────────
_KEY = "_pio_serial_spawn_lock"
if _KEY not in sys.modules:
    _mod = type(sys)(_KEY)
    _mod.sem = threading.Semaphore(1)
    sys.modules[_KEY] = _mod

_sem = sys.modules[_KEY].sem

# ── Wrap SPAWN in this environment ──────────────────────────────────────────
_orig_spawn = env["SPAWN"]

def _serial_spawn(sh, escape, cmd, args, spawnenv):
    with _sem:
        return _orig_spawn(sh, escape, cmd, args, spawnenv)

env["SPAWN"] = _serial_spawn
env.SetOption("num_jobs", 1)   # hint (may be overridden by PlatformIO, that's OK)

print("Build parallelism limited to 1 (Windows MSYS2 compatibility)")
