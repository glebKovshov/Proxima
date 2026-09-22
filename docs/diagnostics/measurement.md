# Measurement model

`NetworkMeasurementEngine` records successful RTT samples and failed probes in one bounded rolling window. It derives:

- current/minimum/average/maximum RTT from valid samples;
- jitter as the mean absolute delta between neighboring samples;
- packet loss as failed probes divided by total probes;
- a preliminary stability score for presentation.

`RouteScoringEngine` then combines RTT, jitter, packet loss, stability, and sample confidence using configurable weights. A single probe never determines route quality; confidence increases toward 1.0 as the configured sample count is reached.

Traceroute and DNS are diagnostic tools only. A timeout at an individual hop is reported as a timeout and is not automatically treated as a route failure.
