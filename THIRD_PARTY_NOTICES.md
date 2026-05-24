# Third-Party Notices

Keyflow incorporates the following third-party components. Each is used
under its own license, listed below. Keyflow itself is licensed under
the MIT License (see [LICENSE](LICENSE)) — this file documents the
additional obligations that come with the bundled dependencies.

## Interception

- **Source:** https://github.com/oblitum/Interception
- **License:** GNU Lesser General Public License, version 3 (LGPL-3.0)
- **Bundled files:**
  - `src/platform/windows/interception.dll`
  - `src/platform/windows/interception.h`
  - `src/platform/windows/interception.lib`
- **Author:** Francisco Lopes (oblitum) and contributors

Interception is a low-level keyboard/mouse driver shim for Windows.
Keyflow links against it dynamically (the `.dll` is loaded at runtime
via the import library), so end users can replace `interception.dll`
with a modified build of Interception without rebuilding keyflow —
satisfying the LGPL-3.0 requirement that recipients be able to use a
modified version of the library.

A full copy of the LGPL-3.0 license text is available at:
https://www.gnu.org/licenses/lgpl-3.0.txt

Corresponding source for the version bundled here is available at the
project URL above. If you redistribute keyflow, you must include this
notice and either bundle the LGPL-3.0 license text or link to it.

## nlohmann/json

- **Source:** https://github.com/nlohmann/json
- **Version:** 3.11.3
- **License:** MIT
- **Bundled file:** `src/external/nlohmann/json.hpp`
- **Copyright:** 2013-2023 Niels Lohmann <https://nlohmann.me>

```
MIT License

Copyright (c) 2013-2023 Niels Lohmann <https://nlohmann.me>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
