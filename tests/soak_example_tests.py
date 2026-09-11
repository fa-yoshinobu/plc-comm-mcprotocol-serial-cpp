"""Exercise the real shell function with fake reads/writes; no PLC I/O."""
from pathlib import Path
import os
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
candidates = [shutil.which("bash")]
if os.name == "nt":
    candidates = ["C:/Program Files/Git/bin/bash.exe", "C:/msys64/usr/bin/bash.exe"]
bash = next((p for p in candidates if p and Path(p).is_file()), None)
if not bash:
    print("SKIP: Bash required")
    sys.exit(77)
source = (root / "examples/linux_cli/supported_device_rw_soak.sh").read_text()
functions = source[source.index("format_value() {"):source.index('if [[ ! "${duration_sec}"')]
for readback, restored, expected in [(11, 10, 0), (99, 10, 1), (11, 88, 1), (99, 88, 1)]:
    script = f'''reads=0
read_scalar() {{ reads=$((reads+1)); case "$reads" in 1) REPLY=10;; 2) REPLY={readback};; 3) REPLY={restored};; esac; }}
write_scalar() {{ return 0; }}
''' + functions + '\nexercise_target word D100\n'
    result = subprocess.run([bash, "--noprofile", "--norc"], input=script, text=True, capture_output=True)
    if result.returncode != expected:
        raise RuntimeError(f"Unexpected result: {result.returncode}: {result.stdout} {result.stderr}")
print("Soak success/mismatch checks: PASS")
