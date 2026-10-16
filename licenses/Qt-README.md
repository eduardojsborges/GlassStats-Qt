# Qt redistribution

The remake dynamically links LGPL-compatible Qt 6.12.0 modules (Core, Gui, Widgets, Network, Concurrent, Qml/Quick/Quick Controls, SVG and platform plugins). It does not use Qt Charts/Graphs/Quick3D. Keep each module's LGPL/GPL exceptions and third-party notices with distributed packages; review the generated Qt SBOM in the SDK.

Qt source: https://download.qt.io/official_releases/qt/6.12/6.12.0/ and https://code.qt.io/cgit/qt/

License texts: https://code.qt.io/cgit/qt/qtbase.git/tree/LICENSES?h=v6.12.0

Users may replace the dynamically linked Qt libraries with compatible modified versions. This application imposes no prohibition on reverse engineering for debugging such modifications. GlassStats's own original code remains MIT licensed.

Before a public binary release, provide corresponding source access for the exact Qt modules/third-party libraries shipped, alongside the release assets, and retain the build/deployment instructions and upstream notices. A checksum list alone is not a substitute for source or license materials. CI artifacts are development previews; verify their redistribution materials before publishing an installer as a release.
