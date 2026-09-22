# Architecture overview

Proxima is intentionally split into contracts that do not depend on the Qt UI:

| Layer | Responsibility |
| --- | --- |
| UI | Qt widgets, status presentation, tray, user actions |
| Application | lifecycle orchestration and asynchronous work |
| Core | `Route`, `RouteMetrics`, profiles, endpoint and health value types |
| Diagnostics | ICMP sampling, rolling statistics, jitter, loss, traceroute, DNS |
| Discovery | Fortnite process and network endpoint discovery |
| Routing | scoring, route manager, best-route selection |
| Transport | `ITransport`, `DirectTransport`, future relay stub |
| Backend | HTTPS payload validation, cache fallback, version/config contracts |
| Platform | Windows Toolhelp, IP Helper, ICMP, system information |

The v0.1 client never changes game traffic. Future routing may be implemented behind a service and a concrete `RelayTransport`; the route metrics and UI contracts remain unchanged.
