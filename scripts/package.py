"""Build the standard NaiwaGuo jailbreak package."""
import io
import pathlib
import tarfile

root = pathlib.Path(__file__).resolve().parent.parent


def archive(files):
    stream = io.BytesIO()
    with tarfile.open(fileobj=stream, mode="w:gz", format=tarfile.USTAR_FORMAT) as tar:
        for name, content, mode in files:
            item = tarfile.TarInfo(name)
            item.size = len(content)
            item.mode = mode
            item.mtime = 0
            tar.addfile(item, io.BytesIO(content))
    return stream.getvalue()


version = next(
    line.split(":", 1)[1].strip()
    for line in (root / "control").read_text().splitlines()
    if line.startswith("Version:")
)


def build_package(rootless=False):
    control_text = (root / "control").read_text()
    architecture = "iphoneos-arm64" if rootless else "iphoneos-arm"
    if rootless:
        control_text = control_text.replace("Architecture: iphoneos-arm", "Architecture: iphoneos-arm64")
    control = archive([("./control", control_text.encode(), 0o644)])
    base = "./var/jb/Library/MobileSubstrate/DynamicLibraries/" if rootless else "./Library/MobileSubstrate/DynamicLibraries/"
    files = [
        (base + "NaiwaGuo.dylib", (root / "dist/NaiwaGuo.dylib").read_bytes(), 0o755),
        (base + "NaiwaGuo.plist", (root / "NaiwaGuo.plist").read_bytes(), 0o644),
    ]
    documentation = "./var/jb/usr/share/doc/com.naiwaguo.native/" if rootless else "./usr/share/doc/com.naiwaguo.native/"
    for name in ["LICENSE", "THIRD_PARTY_NOTICES.md", "DISCLAIMER.md", "third_party/DYKiller-LICENSE", "third_party/fishhook-LICENSE"]:
        files.append((documentation + name, (root / name).read_bytes(), 0o644))
    data = archive(files)
    out = root / f"dist/com.naiwaguo.native_{version}_{architecture}.deb"
    with out.open("wb") as file:
        file.write(b"!<arch>\n")
        for name, content in (
            ("debian-binary", b"2.0\n"),
            ("control.tar.gz", control),
            ("data.tar.gz", data),
        ):
            header = (
                f"{name:<16}{0:<12}{0:<6}{0:<6}{100644:<8}"
                f"{len(content):<10}`\n"
            ).encode("ascii")
            assert len(header) == 60
            file.write(header)
            file.write(content)
            if len(content) % 2:
                file.write(b"\n")
    return out


print(build_package())
print(build_package(rootless=True))
