# Third-Party Dependencies and Licences

This document records all third-party libraries, header files, and assets integrated or planned for the Unified Engineering Workbench, including their version, licence, upstream source, and vendoring method.

---

## 1. C++ Dependencies

### doctest
- **Version:** 2.4.11
- **Licence:** MIT Licence
- **Source URL:** https://github.com/doctest/doctest
- **How Vendored:** Vendored as a single self-contained header file at `third_party/doctest/doctest.h`.
- **Purpose:** Fast, lightweight, header-only C++17 unit testing framework.

### nlohmann/json (Planned for Task A Server)
- **Version:** 3.11.3
- **Licence:** MIT Licence
- **Source URL:** https://github.com/nlohmann/json
- **How Vendored:** Header-only single file `third_party/nlohmann/json.hpp`.
- **Purpose:** Modern JSON parsing and serialization for HTTP REST transport.

### cpp-httplib (Planned for Task A Server)
- **Version:** 0.18.3
- **Licence:** MIT Licence
- **Source URL:** https://github.com/yhirose/cpp-httplib
- **How Vendored:** Header-only single file `third_party/httplib.h`.
- **Purpose:** Lightweight C++ HTTP server for localhost IPC.

---

## 2. Frontend Dependencies (Vendored for Offline Execution)

### Three.js
- **Version:** r128 (minified, standalone)
- **Licence:** MIT Licence
- **Source URL:** https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js
- **How Vendored:** Vendored as a static file in `web/js/three.min.js` (589.3 KB; offline; zero CDN or network required).
- **Purpose:** 3D rendering for ASCII STL geometry and ANSYS CSV point cloud in WebGL.

---

## 3. Licence Notices Summary
All third-party components utilized are distributed under permissive open-source licences (MIT), permitting inclusion, compilation, and redistributable packaging without copyleft obligations.
