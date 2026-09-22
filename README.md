# Proxima

Proxima is a Windows 10/11 x64 desktop diagnostic client for Fortnite Europe. Version 0.1 measures route quality without modifying user traffic and keeps the domain, scoring, and transport contracts ready for future relay routes.

## Build

The desktop target requires Qt 6.4+ with `Core`, `Widgets`, `Network`, and `Concurrent` modules plus a C++20 compiler. On a Windows developer prompt:

```powershell
cmake -S . -B build -G Ninja -DPROXIMA_BUILD_DESKTOP=ON -DPROXIMA_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

The core and tests can be configured without Qt, which is useful for CI or a headless development machine:

```powershell
cmake -S . -B build-core -G Ninja -DPROXIMA_BUILD_DESKTOP=OFF -DPROXIMA_BUILD_TESTS=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

Set `PROXIMA_BACKEND_URL` to an HTTPS `/v1/config` endpoint to enable remote configuration. If the endpoint is unavailable or its declarative payload fails validation, Proxima falls back to the local cache and then to built-in defaults.

## Architecture

```text
Qt UI
  -> Application orchestration
      -> Core domain / RouteScoringEngine
          -> Diagnostics | Discovery | Routing | Transport
              -> Windows platform APIs
      -> Backend client + validated cache
```

The scoring engine has no Qt dependency. `DirectTransport` is the v0.1 implementation of `ITransport`; `RelayTransportStub` reserves the future relay contract without tunneling or traffic redirection.

## Privacy and scope

The application does not inspect packet contents, require permanent administrator rights, or change routes. Diagnostic exports contain only the explicitly generated metrics, logs, route information, system summary, and version data. Telemetry and accounts are not implemented in v0.1.

## Current limitations

- Windows APIs are used for process discovery, ICMP measurements, DNS, and the initial TCP endpoint discovery implementation.
- A backend server is not bundled; built-in configuration and cache fallback keep the client usable offline.
- Relay tunneling, VPN/TUN/WFP routing, authentication, subscriptions, and auto-installing signed updates are intentionally out of scope for v0.1.
