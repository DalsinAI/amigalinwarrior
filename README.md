# LinWarrior for GFOS

LinWarrior 3D (Benjamin Pickhardt, hackcraft.de; Apache-2.0, its data under
CC-BY, CC-BY-SA, CC0 and Sampling Plus) for AmigaOS 3.2 with GFOpenUp,
68040/68060 + FPU, as a release LhA with an Installer script and as the
GFOpenUp part `Parts/LinWarrior`.

- Upstream: <https://github.com/hackcraft-de/linwarrior> at the commit
  `package/linwarrior.lock` names, fetched at build time, patched by
  `amiga/patches`.
- SDL 1.2 and OpenGL from GFOpenUp (GFOpenGPU); sound through our own
  OpenAL subset on SDL's audio (`amiga/lw_al.c`, `amiga/lw_al_core.c`);
  GLEW and GLU stand-ins in `amiga/include/GL`; Expat with GFOpenXML's
  68k configuration.

Build (Cradle `stove_build`, target `package`): `sh package/build-amiga.sh`
with `STOVE=` (os32-gcc16-open6) and `OUT=`.

Host tests: `sh tests/host/test_all.sh` (the sound core on the PC and on
68k under qemu-m68k, compared; the archive writer).

Apache-2.0 for the port's own files, like LinWarrior; MIT for `tools/` and
`package/`. See `amiga/Licences.txt`.

Team 3
