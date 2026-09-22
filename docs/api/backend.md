# Backend API notes

The client expects HTTPS and a small declarative JSON document. A valid payload includes `schemaVersion`, `gameId`, and at least one `diagnosticEndpoints` entry. Payloads larger than 1 MiB, malformed documents, unsupported status codes, or non-HTTPS URLs are rejected.

The cache resolution order is:

1. validated remote payload;
2. validated local cache;
3. built-in defaults.

No backend field is interpreted as code or a command.
