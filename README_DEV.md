# Developer Notes

## Prerequisites
- Windows + PowerShell
- CMake + Ninja
- Android NDK r28+ (tested with 28.2.13676358)
- QPM in PATH
- Docker (for Kakasi build)

## Configure NDK
Set one of the following env vars so `cmake/targets/android-ndk.cmake` can find the NDK:

```powershell
$env:ANDROID_NDK_HOME = "C:\Android\Sdk\ndk\28.2.13676358"
# or
$env:ANDROID_NDK_LATEST_HOME = "C:\Android\Sdk\ndk\28.2.13676358"
```

## Restore Dependencies
```powershell
qpm restore -u *> qpm-restore.log
```

## Kakasi (Romaji) Library Build
The Dockerfile with `-fPIC` is in `third-party/kakasi/Dockerfile`.
It builds Debian's patched source twice: first on host (with `ja_JP.EUC-JP` locale) to generate dictionaries, then for Android.
It also builds GNU libiconv for Android so EUC-JP conversions work on device.

```powershell
cd .\third-party\kakasi
docker build -f Dockerfile -t kakasi-pic .
$cid = docker create kakasi-pic
docker cp "$cid`:/build/output" .\_docker_output
docker rm $cid
Copy-Item -Recurse -Force .\_docker_output\output\* .\
```

The adapter expects:
- `third-party/kakasi/lib/libkakasi.a`
- `third-party/kakasi/lib/libiconv.a`
- `third-party/kakasi/lib/libcharset.a`
- `third-party/kakasi/include/libkakasi.h`
- `third-party/kakasi/share/kakasi/kanwadict`
- `third-party/kakasi/share/kakasi/itaijidict`

## Build
```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## Package (.qmod)
```powershell
powershell -ExecutionPolicy Bypass -File scripts\createqmod.ps1
```
