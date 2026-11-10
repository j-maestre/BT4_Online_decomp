#!/usr/bin/env python3
"""Publica una version nueva de BT4 Online (entrenador + Historia cooperativa) en GitHub.

    python fork/publicar.py 1.0.1 "Lo que cambia en esta version"

Hace, por orden:
  1. Compila el juego con ese numero de version (MSYS2 UCRT64, port/build/build_win.sh).
  2. Monta el zip BT4decomp-windows-x64.zip: el paquete oficial base (sin datos del disco: el setup lee la ISO de
     cada uno) con nuestro ejecutable, el mod del entrenador y historia_coop/. NUNCA lleva gamedata.
  3. Lo firma con la clave privada (CLAVE_PRIVADA_actualizaciones.txt, fuera del repositorio).
  4. Crea la release v<version> en GitHub y sube el zip y su firma. Los juegos de version anterior la ven al
     arrancar, la descargan, comprueban la firma y se actualizan solos en el menu principal.
  5. Instala el ejecutable nuevo tambien en BT4-entrenador.

Opciones:  --sin-compilar  usa port/build tal como esta (el numero de version tiene que ser el mismo)
           --sin-subir     solo monta y firma el zip (en port/build/publicar/)
           --sin-instalar  no copia el ejecutable nuevo a BT4-entrenador
"""
import argparse, json, os, pathlib, shutil, subprocess, sys, zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
DESK = ROOT.parent                                  # C:\Users\Xema\Desktop\BT4Online
REPO = "j-maestre/BT4_Online_decomp"
BASE_ZIP = ROOT / "fork/base/BT4decomp-windows-x64-base.zip"   # el paquete oficial (sin datos del disco)
KEY = DESK / "CLAVE_PRIVADA_actualizaciones.txt"
INSTALL = DESK / "BT4-entrenador"
OUT = ROOT / "port/build/publicar"
ASSET = "BT4decomp-windows-x64.zip"
OURS = {"BT4decomp.exe": "port/build/bt3.exe", "BT4decomp.dat": "port/build/bt3.dat",
        "BT4decomp.exe.mem": "port/build/bt3.exe.mem", "README.txt": "fork/LEEME.txt"}
FOLDERS = {"mods/entrenador_contra_z": "mods_entrenador/entrenador_contra_z", "historia_coop": "historia_coop"}
FORBIDDEN = ("gamedata/", "saves/", "textures/SLUS", "net_session/", "update/")


def build(version):
    env = dict(os.environ, MSYSTEM="UCRT64", CHERE_INVOKING="1")
    cmd = f"export BT3_VERSION={version}; sh {ROOT.as_posix().replace('C:', '/c')}/port/build/build_win.sh"
    r = subprocess.run([r"C:\msys64\usr\bin\bash.exe", "-lc", cmd], env=env)
    if r.returncode:
        sys.exit("La compilacion fallo.")
    gen = (ROOT / "port/build/gen/gs/port_version.h").read_text()
    if f'"{version}"' not in gen:
        sys.exit("El ejecutable no lleva el numero de version pedido.")


def make_zip(version):
    OUT.mkdir(parents=True, exist_ok=True)
    out = OUT / ASSET
    base = zipfile.ZipFile(BASE_ZIP)
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for info in base.infolist():
            rel = info.filename[len("BT4decomp/"):]
            if rel in OURS or info.is_dir():
                continue
            z.writestr(info, base.read(info))
        for rel, src in OURS.items():
            z.write(ROOT / src, "BT4decomp/" + rel)
        for rel, src in FOLDERS.items():
            for f in sorted((ROOT / src).rglob("*")):
                if f.is_file():
                    z.write(f, f"BT4decomp/{rel}/{f.relative_to(ROOT / src).as_posix()}")
        z.writestr("BT4decomp/FORK_VERSION.txt", f"{version}\n")
    with zipfile.ZipFile(out) as z:
        bad = [n for n in z.namelist() if any(n[len("BT4decomp/"):].lower().startswith(f) for f in FORBIDDEN)]
        if bad:
            out.unlink()
            sys.exit(f"El zip llevaba archivos que no pueden salir del PC: {bad[:5]}")
    print(f"zip: {out} ({out.stat().st_size:,} bytes)")
    return out


def sign(zip_path):
    r = subprocess.run([sys.executable, "-X", "utf8", str(ROOT / "port/tools/update_key.py"), "sign", str(zip_path),
                        "--key-file", str(KEY)])
    if r.returncode:
        sys.exit("No se pudo firmar el zip.")
    r = subprocess.run([sys.executable, "-X", "utf8", str(ROOT / "port/tools/update_key.py"), "verify", str(zip_path)])
    if r.returncode:
        sys.exit("La firma no coincide con la clave del juego.")
    return zip_path.with_name(zip_path.name + ".sig")


def token():
    r = subprocess.run(["git", "credential", "fill"], input="protocol=https\nhost=github.com\n\n",
                       capture_output=True, text=True)
    for line in r.stdout.splitlines():
        if line.startswith("password="):
            return line[9:]
    sys.exit("No hay sesion de GitHub guardada en este PC (haz un git push una vez).")


def api(method, url, tok, data=None, ctype="application/json"):
    """HTTPS through Windows' own curl.exe (its certificate store: Python's alone sometimes fails on GitHub's chain).
    The token goes in a temporary header file, never on the command line."""
    import tempfile
    body = json.dumps(data).encode() if isinstance(data, dict) else data
    with tempfile.TemporaryDirectory() as tmp:
        head = pathlib.Path(tmp) / "h.txt"
        head.write_text(f"Authorization: Bearer {tok}\nAccept: application/vnd.github+json\n"
                        f"Content-Type: {ctype}\nUser-Agent: bt4-fork-publisher\n")
        cmd = [os.environ.get("SystemRoot", r"C:\Windows") + r"\System32\curl.exe", "-sS", "--retry", "3",
               "-X", method, "-H", f"@{head}", "-w", "\n%{http_code}", url]
        if body is not None:
            src = pathlib.Path(tmp) / "body.bin"
            src.write_bytes(body)
            cmd += ["--data-binary", f"@{src}"]
        r = subprocess.run(cmd, capture_output=True)
    out = r.stdout.decode(errors="replace")
    text, _, code = out.rpartition("\n")
    if r.returncode or not code.startswith("2"):
        sys.exit(f"GitHub respondio {code or r.returncode}: {(text or r.stderr.decode(errors='replace'))[:400]}")
    return json.loads(text or "{}")


def publish(version, notes, files):
    tok = token()
    head = subprocess.run(["git", "-C", str(ROOT), "rev-parse", "HEAD"], capture_output=True, text=True).stdout.strip()
    rel = api("POST", f"https://api.github.com/repos/{REPO}/releases", tok, {
        "tag_name": f"v{version}", "target_commitish": head, "name": f"BT4 Online {version}",
        "body": notes, "draft": False, "prerelease": False})
    upload = rel["upload_url"].split("{")[0]
    for f in files:
        api("POST", f"{upload}?name={f.name}", tok, f.read_bytes(), "application/octet-stream")
        print(f"subido: {f.name}")
    print(f"release: {rel['html_url']}")


def install_local():
    for rel, src in OURS.items():
        if rel != "README.txt":
            shutil.copy2(ROOT / src, INSTALL / rel)
    print(f"instalado en {INSTALL}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("version")
    ap.add_argument("notas", nargs="?", default="")
    ap.add_argument("--sin-compilar", action="store_true")
    ap.add_argument("--sin-subir", action="store_true")
    ap.add_argument("--sin-instalar", action="store_true", help="no copia el ejecutable a BT4-entrenador")
    a = ap.parse_args()
    if not KEY.exists():
        sys.exit(f"Falta la clave privada: {KEY}")
    if not a.sin_compilar:
        build(a.version)
    z = make_zip(a.version)
    s = sign(z)
    if not a.sin_subir:
        publish(a.version, a.notas or f"Version {a.version}", [z, s])
    if not a.sin_instalar:
        install_local()


if __name__ == "__main__":
    main()
