"""Pydantic models for the unified telemetry payload (spec section 6.1)."""
from datetime import datetime
from typing import List, Literal, Optional

from pydantic import BaseModel, Field, model_validator


class SensorReadings(BaseModel):
    temperature_dht: Optional[float] = Field(None, ge=-40, le=85)
    humidity: Optional[float] = Field(None, ge=0, le=100)
    temperature_bmp: Optional[float] = Field(None, ge=-40, le=85)
    pressure: Optional[float] = Field(None, ge=300, le=1100)
    door: Literal["open", "closed"]

    @model_validator(mode="after")
    def at_least_one_temperature(self):
        if self.temperature_dht is None and self.temperature_bmp is None:
            raise ValueError("at least one of temperature_dht or temperature_bmp is required")
        return self


class RfidRead(BaseModel):
    reader_id: Literal[1, 2]
    tag_uid: str = Field(..., min_length=1, max_length=32)
    product: Optional[str] = None


class TelemetryPayload(BaseModel):
    device_id: str = Field(..., min_length=1, max_length=64)
    timestamp: datetime
    connection_type: Literal["MQTT", "HTTPS"]
    battery_level: Optional[int] = Field(None, ge=0, le=100)
    telemetry: SensorReadings
    rfid: List[RfidRead] = Field(default_factory=list, max_length=2)


class TelemetryRecord(BaseModel):
    """One row of the telemetry table, as returned by the API."""
    id: int
    device_id: str
    temperature: Optional[float]
    humidity: Optional[float]
    pressure: Optional[float]
    battery_level: Optional[int]
    connection_type: str
    is_alert: bool
    recorded_at: datetime
