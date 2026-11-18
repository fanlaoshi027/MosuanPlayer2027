# Third-party licensing plan

## Product licensing model
MosuanPlayer application code is intended to remain closed-source. Third-party components keep their original licenses. This plan is an engineering checklist, not legal advice.

## Media engine: libmpv
- The selected engine is libmpv, dynamically loaded as libmpv-2.dll.
- mpv is GPLv2+ by default. The mpv project documents an LGPLv2.1+ build mode when GPL-only files are excluded using -Dgpl=false. The build switch alone is not proof: inspect the actual build and all linked dependencies.
- Build FFmpeg without GPL-only and nonfree components. Verify its configure flags and exact source revision.
- Prefer a replaceable shared libmpv DLL. Do not statically merge libmpv into MosuanPlayer.
- Include the exact corresponding source for libmpv and LGPL components, build scripts/configuration, license texts, copyright notices and any required modification notices.
- Preserve LGPL rights to replace/relink the library and reverse-engineer solely for debugging modifications to the LGPL library, subject to the applicable license terms.
- Record exact upstream commits, build recipe, compiler, dependency versions, DLL hashes and source archive hashes for every shipped release.
- Never bundle an arbitrary third-party DLL merely because its filename says LGPL.

## Current integration
The application dynamically loads libmpv-2.dll and uses mpv's native-window embedding option. The current CI package does not yet include a verified libmpv DLL. The player therefore reports a missing engine at runtime until the audited LGPL binary is added to the package.

## Qt
Use only Qt modules whose selected license permits the intended distribution. For LGPL Qt components, dynamically link and fulfill applicable notices, source, relinking and reverse-engineering requirements.

## Release gate
No commercial release until the exact shipped binaries and fulfillment of all license obligations have been reviewed.
