# Third-party notices

NavigateVR Map Framework's original source is licensed under the MIT License.
The compiled DLL also incorporates third-party software.

## CommonLibSSE-NG

This project statically links the pinned `ng` branch of CommonLibSSE-NG.
That revision is licensed under GPL-3.0-or-later with the Modding Exception and
GPL-3.0 Linking Exception (with Corresponding Source). Its full GPL text and
exceptions are distributed with the binary package under
`THIRD_PARTY_LICENSES/`.

Corresponding source, including the pinned CommonLibSSE-NG submodule, is
available from:

https://github.com/LivSterling/NavigateVR-Map-Framework

## Other build dependencies

The pinned build also uses these open-source dependencies through
CommonLibSSE-NG or directly:

- nlohmann/json 3.12.0 — MIT
- spdlog 1.16.0 — MIT
- DirectXMath 2024.02 — MIT
- DirectX Tool Kit 24.2.0 — MIT
- rapidcsv 8.92 — BSD-3-Clause

Their source and license information are available through the recursive
source tree and XMake package declarations. No endorsement by these projects
is implied.
