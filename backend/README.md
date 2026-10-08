# Backend — Phase 1

Receives telemetry from the ESP32 over MQTT (primary) or HTTPS (backup), stores it in PostgreSQL,
and serves it to the dashboard.

```text
ESP32 --MQTT--> Mosquitto --> mqtt_subscriber --+
      --HTTPS (backup)------> FastAPI ----------+--> save_reading() --> PostgreSQL --> FastAPI --> Dashboard
```

Both transports go through the same `save_reading()` in `app/service.py`, which:

1. validates the payload (`app/schemas.py`, spec section 6.1),
2. computes `temperature` = mean of `temperature_dht` and `temperature_bmp` (or the one available),
3. sets `is_alert` when the temperature is outside 2–8 °C,
4. inserts one row into the `telemetry` table (`../db/schema.sql`).

## Folder layout

```text
backend/
  app/
    main.py             FastAPI app (+ /health)
    routers/telemetry.py  telemetry endpoints
    mqtt_subscriber.py  MQTT listener daemon
    service.py          shared processing logic
    schemas.py          Pydantic models
    db.py               PostgreSQL queries
    config.py           settings from environment / .env
  tests/                pytest tests (no database needed)
  mosquitto/            broker config
  docker-compose.yml    db + mqtt + api + subscriber
  sample_payload.json   example ESP32 message
```

## Run with Docker (easiest)

```bash
cd backend
docker compose up --build
```

- API and docs: http://localhost:8000/docs
- PostgreSQL: localhost:**5433** (user/password/db: `coldchain`). Port 5433, not 5432, so it does not clash
  with a PostgreSQL installed on Windows.
- MQTT broker: localhost:1883

## Run the API outside Docker (database still in Docker)

Handy while developing, because `--reload` restarts the API when you save a file.

```bash
docker compose up -d db mqtt          # only the database and broker
.venv\Scripts\activate
uvicorn app.main:app --reload         # uses localhost:5433 by default
python -m app.mqtt_subscriber         # in another terminal
```

Do not also start the `api` container then, or both will try to use port 8000.

## Run without Docker

Needs Python 3.10+, a running PostgreSQL and a running Mosquitto.

```bash
cd backend
python -m venv .venv
.venv\Scripts\activate            # Windows  (Linux/macOS: source .venv/bin/activate)
pip install -r requirements-dev.txt
copy .env.example .env            # Linux/macOS: cp .env.example .env
# edit .env: set DATABASE_URL to your own PostgreSQL, usually ...@localhost:5432/coldchain
psql -U postgres -c "CREATE USER coldchain WITH PASSWORD 'coldchain'"
psql -U postgres -c "CREATE DATABASE coldchain OWNER coldchain"
psql -U coldchain -d coldchain -f ../db/schema.sql

uvicorn app.main:app --reload            # terminal 1: API on port 8000
python -m app.mqtt_subscriber            # terminal 2: MQTT listener
```

## Try it

Send a reading over HTTPS (the backup path):

```bash
curl -X POST http://localhost:8000/api/v1/telemetry/backup \
  -H "Content-Type: application/json" -H "X-API-Key: change-me" \
  -d @sample_payload.json
```

Send a reading over MQTT:

```bash
mosquitto_pub -h localhost -t coldchain/VAULT_FRIDGE_01/telemetry -q 1 -f sample_payload.json
```

Read it back:

```bash
curl http://localhost:8000/api/v1/telemetry/latest
curl "http://localhost:8000/api/v1/telemetry?device_id=VAULT_FRIDGE_01&limit=50"
```

## Endpoints

| Method | Path | Auth | Purpose |
| --- | --- | --- | --- |
| GET | `/health` | — | API and database status |
| POST | `/api/v1/telemetry` | `X-API-Key` | Store a reading over HTTPS (also useful for testing) |
| POST | `/api/v1/telemetry/backup` | `X-API-Key` | Failover path used by the ESP32 when MQTT fails; returns 201 |
| GET | `/api/v1/telemetry/latest` | — | Most recent reading (`?device_id=` optional) |
| GET | `/api/v1/telemetry` | — | History, newest first (`?device_id=&from=&to=&limit=`) |

Errors: `401` wrong or missing API key, `422` invalid payload.

## MQTT

- Topic: `coldchain/<device_id>/telemetry`, QoS 1
- Payload: same JSON as the HTTPS endpoints (see `sample_payload.json`)
- Invalid messages are logged and skipped.
- Development uses plain MQTT on 1883. The spec requires TLS on 8883 — see `mosquitto/mosquitto.conf`
  and set `MQTT_TLS=true`, `MQTT_PORT=8883`, `MQTT_CA_CERT=...` in `.env`.

## Tests

```bash
pytest
```

The tests use an in-memory fake instead of PostgreSQL, so they run without Docker.

## Not in phase 1

RFID inventory tracking, door events, alerts table, WebSocket live updates, ML inference,
replay of readings buffered on the ESP32, and dashboard login.
