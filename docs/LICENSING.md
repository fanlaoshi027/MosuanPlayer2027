# Third-party licensing policy

## Product requirement
MosuanPlayer application code is intended to remain proprietary. No third-party commercial license purchases are planned. This is an engineering compliance plan, not legal advice or a final legal opinion.

## Qt
Use only Qt modules available under LGPL terms for the selected Qt release. Prefer dynamic linking. Avoid Qt modules available only under GPL for open-source users. Ship required license texts, corresponding Qt source (including modifications), notices and a practical relinking/replacement mechanism as required by the applicable LGPL terms.

## FFmpeg
FFmpeg is generally LGPL 2.1-or-later, but optional components can change the license. Build without `--enable-gpl` and without `--enable-nonfree`; dynamically link DLLs. Audit every enabled external library and codec. Preserve exact source corresponding to distributed binaries, patches, build configuration and notices. Do not enable GPL codecs/filters or non-redistributable components.

## Prohibited assumptions
- Do not assume every FFmpeg build is LGPL.
- Do not assume Qt Multimedia or any prebuilt bundle has a simple license without auditing its exact plugin/dependency set.
- Do not use libmpv in the proprietary build path; mpv is GPL-licensed.
- Do not ship unknown prebuilt DLLs.

## Required dependency manifest
For every shipped component record: name, exact version/commit, upstream URL, license/SPDX identifier, binary SHA-256, build flags, patches, source archive location, notices, and dynamic/static linkage.

## Release gate
No release artifact may be labeled commercially redistributable until a human-reviewed dependency audit confirms all shipped binaries and obligations.
