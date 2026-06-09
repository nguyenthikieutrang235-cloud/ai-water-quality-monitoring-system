from __future__ import annotations

import re
from typing import Any, Dict, Optional

LABEL_ALIASES = {
    "sach": "sach",
    "sạch": "sach",
    "clean": "sach",
    "0": "sach",
    0: "sach",
    "ban": "ban",
    "bẩn": "ban",
    "dirty": "ban",
    "1": "ban",
    1: "ban",
    "2": "ban",
    2: "ban",
    # Giữ tương thích dữ liệu cũ: mọi giá trị cảnh báo sẽ được gom về "ban"
    "canh_bao": "ban",
    "cảnh_báo": "ban",
    "cảnh báo": "ban",
    "warning": "ban",
}

DISPLAY_LABELS = {
    "sach": "SẠCH",
    "ban": "BẨN",
}

UART_PATTERN = re.compile(
    r"PPM:(?P<ppm>-?\d+(?:\.\d+)?),"
    r"NTU:(?P<ntu>-?\d+(?:\.\d+)?),"
    r"D:(?P<distance>ERR|-?\d+(?:\.\d+)?),"
    r"Q:(?P<q>\d+),"
    r"F:(?P<f>\d+),"
    r"H:(?P<h>\d+),"
    r"S:(?P<s>\d+),"
    r"P:(?P<p>\d+),"
    r"PULSE:(?P<pulse>\d+)"
)


def normalize_label(value: Any) -> str:
    key = str(value).strip().lower() if value is not None else ""
    if key in LABEL_ALIASES:
        return LABEL_ALIASES[key]
    raise ValueError('Nhãn không hợp lệ. Dùng một trong các giá trị: sach, ban')


def display_label(value: str) -> str:
    return DISPLAY_LABELS.get(value, value.upper())


def rule_label(ppm: float, ntu: float) -> str:
    """
    Rule nhị phân theo yêu cầu mới:
    - sạch nếu PPM <= 300 và NTU <= 220
    - bỏ ngưỡng cảnh báo, nên mọi trường hợp còn lại đều là bẩn
    """
    if ppm <= 300 and ntu <= 220:
        return "sach"
    return "ban"


def parse_uart_line(line: str) -> Optional[Dict[str, Any]]:
    match = UART_PATTERN.search(line.strip())
    if not match:
        return None

    distance_raw = match.group("distance")
    distance_cm = None if distance_raw == "ERR" else float(distance_raw)

    return {
        "ppm": float(match.group("ppm")),
        "ntu": float(match.group("ntu")),
        "distance_cm": distance_cm,
        "quality_code": int(match.group("q")),
        "fill_request": int(match.group("f")),
        "display_hold": int(match.group("h")),
        "show_due": int(match.group("s")),
        "pump_state": int(match.group("p")),
        "pulse_us": int(match.group("pulse")),
        "raw": line.strip(),
    }
