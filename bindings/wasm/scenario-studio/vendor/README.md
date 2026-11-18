AVENC reader modules are copied from https://github.com/filippi/avenc (browser reader sources).
The modules use a local fflate import. The range reader also has an opt-in
`applyBitmap:false` setting: ForeFire uses this to retain the archive owner's
encoded damped ocean wind values while preserving bitmap-aware dequantization.
The default reader behavior still applies the bitmap.
See AVENC-LICENSE.

- fflate 0.8.2: https://cdn.jsdelivr.net/npm/fflate@0.8.2/esm/browser.js (MIT)
- MP4Box.js 0.5.3: https://cdn.jsdelivr.net/npm/mp4box@0.5.3/dist/mp4box.all.min.js (BSD-3-Clause)

CesiumJS 1.114.0 is loaded from unpkg; DM Sans and Space Grotesk from Google Fonts.
ForeFire's tile transport and compiled engine use the repository root LICENSE.

- polygon-clipping 0.15.7 is reused from Calormat's vendored isochrone worker.
  Its MIT license and bundled splaytree, robust-predicates and TypeScript helper
  licenses are included alongside it. No additional CDN request is needed.
