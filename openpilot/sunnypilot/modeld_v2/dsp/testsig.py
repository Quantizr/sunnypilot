"""Ensure the Qualcomm cDSP testsig is installed so the unsigned megakernel PD can load.

The signature is keyed by the device serial and lives at /dsp/cdsp/testsig-0x<serial>.so. It is WIPED ON
REBOOT, so modeld checks for it at startup and regenerates+installs it (via the vendored generator) if
missing. Installing requires remounting /dsp read-write, which needs root; on the comma device sudo is
passwordless.
"""

import importlib.util
import os
import subprocess
import tempfile

SERIAL_PATH = "/sys/devices/soc0/serial_number"
CDSP_DIR = "/dsp/cdsp"

def _load_sign():
  """Import _sign from the tinygrad submodule's generator (extra/testsig has no __init__, so load by path).
  Walk up from this file to the openpilot repo root that contains tinygrad_repo/ (host or device checkout)."""
  d = os.path.dirname(os.path.abspath(__file__))
  for _ in range(8):
    cand = os.path.join(d, "tinygrad_repo", "extra", "testsig", "generate_testsig.py")
    if os.path.isfile(cand):
      spec = importlib.util.spec_from_file_location("generate_testsig", cand)
      mod = importlib.util.module_from_spec(spec)
      spec.loader.exec_module(mod)
      return mod._sign
    d = os.path.dirname(d)
  raise FileNotFoundError("tinygrad_repo/extra/testsig/generate_testsig.py not found")

def _serial():
  with open(SERIAL_PATH) as f:
    return int(f.read().strip(), 0)

def testsig_path(serial=None):
  if serial is None:
    serial = _serial()
  return os.path.join(CDSP_DIR, f"testsig-0x{serial:08x}.so")

def _cwd_symlink(dst, serial):
  """tinygrad's DSP RPCListener stats the testsig by a RELATIVE name resolved against the process CWD, so a
  `./testsig-0x<serial>.so` symlink must exist in modeld's CWD or open_lib FileNotFoundErrors. Best-effort."""
  link = os.path.join(os.getcwd(), f"testsig-0x{serial:08x}.so")
  try:
    if os.path.realpath(link) != os.path.realpath(dst):
      if os.path.islink(link) or os.path.exists(link):
        os.remove(link)
      os.symlink(dst, link)
  except OSError as e:
    print(f"[testsig] could not create CWD symlink {link} ({e}); open_lib may need it")

def ensure_testsig():
  """Return True if the testsig is present (installing it if missing). Non-fatal: returns False and logs on
  failure so the caller can decide (the DSP open_lib will fail loudly downstream if it's really absent)."""
  try:
    serial = _serial()
  except OSError as e:
    print(f"[testsig] cannot read {SERIAL_PATH} ({e}); assuming not a Hexagon device")
    return False
  dst = testsig_path(serial)
  if os.path.exists(dst):
    _cwd_symlink(dst, serial)
    return True
  print(f"[testsig] {dst} missing (wiped on reboot?) — generating + installing")
  _sign = _load_sign()
  with tempfile.TemporaryDirectory() as td:
    src = _sign(serial, td)
    os.chmod(td, 0o755)
    os.chmod(src, 0o644)
    sudo = [] if os.geteuid() == 0 else ["sudo", "-n"]
    try:
      subprocess.run(sudo + ["mount", "-o", "remount,rw", "/dsp"], check=True)
      subprocess.run(sudo + ["cp", src, dst], check=True)
      subprocess.run(sudo + ["chmod", "644", dst], check=False)
    except (subprocess.CalledProcessError, PermissionError, OSError) as e:
      print(f"[testsig] install FAILED ({e}); provision manually: sudo mount -o remount,rw /dsp && sudo cp {src} {dst} && sudo mount -o remount,ro /dsp")
      return False
    finally:
      subprocess.run(sudo + ["mount", "-o", "remount,ro", "/dsp"], check=False)
  ok = os.path.exists(dst)
  print(f"[testsig] {'installed ' + dst if ok else 'install did not land'}")
  if ok:
    _cwd_symlink(dst, serial)
  return ok

if __name__ == "__main__":
  print("testsig present:", ensure_testsig())
