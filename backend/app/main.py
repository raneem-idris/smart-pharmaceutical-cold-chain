"""FastAPI application entry point.

Run locally:  uvicorn app.main:app --reload   (from the backend/ folder)
API docs:     http://localhost:8000/docs
"""
import logging

import psycopg
from fastapi import Depends, FastAPI, Request
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware

from .db import get_repo
from .routers import telemetry

app = FastAPI(title="Cold-Chain Warehouse Backend", version="0.1.0")

# The React dashboard runs on a different port during development.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://localhost:5173"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(telemetry.router)

log = logging.getLogger("app")


@app.exception_handler(psycopg.OperationalError)
def database_unavailable(request: Request, exc: psycopg.OperationalError):
    log.error("Database unavailable: %s", str(exc).splitlines()[0])
    return JSONResponse(
        status_code=503,
        content={"detail": "Database unavailable. Check that PostgreSQL is running and DATABASE_URL is correct."},
    )


@app.get("/health", tags=["health"])
def health(repo=Depends(get_repo)):
    try:
        repo.ping()
        database = "ok"
    except Exception:
        database = "unreachable"
    return {"status": "ok", "database": database}
