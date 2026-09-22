# Proxima control-plane contract

The client consumes a declarative JSON document over HTTPS. The server must not send executable content or shell commands. A payload is size-limited and schema-checked before it can affect the client.

Suggested endpoints:

- `GET /v1/config`
- `GET /v1/games/fortnite`
- `GET /v1/endpoints/fortnite/europe`
- `GET /v1/version`
- `GET /v1/relays` (future)

If the request fails, the client uses the last valid cache and finally its built-in default configuration.
