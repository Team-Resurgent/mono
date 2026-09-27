# `teamresurgent` — CI-only branch

This branch contains **no Mono source**. Its only job is CI: it builds the **Xbox port of
Mono** from the [`xbox`](../../tree/xbox) branch and publishes a prebuilt **libmono** package
as a rolling GitHub release, which [**RXDK-DotNet**](https://github.com/Team-Resurgent) consumes
(the same way it consumes the RXDK clang toolchain and RXDK-SDK).

Mirrors the `Team-Resurgent/llvm-project` `teamresurgent` CI-only branch model, which builds its
`xbox` branch and releases the clang toolchain.

- **Source of truth for the port:** the `xbox` branch. Do **not** commit Mono source here.
- **Build recipe:** RXDK-DotNet `docs/phase1-mono.md` (source subset, hand-written config header
  forked from `winconfig.h`, PAL reused from Mono's `HOST_WIN32` path over RXDK-SDK `libxapi`).
- **Workflow:** `.github/workflows/build-libmono.yml` (build steps land once the Phase-1 recipe
  is proven locally).