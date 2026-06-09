from __future__ import annotations

import argparse
import csv
import time
from datetime import datetime
from pathlib import Path

import serial

from model_utils import parse_uart_line, rule_label


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Thu thập dữ liệu UART từ STM32, tự gắn nhãn nhị phân và lưu theo chu kỳ."
    )
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Cổng serial")
    parser.add_argument("--baud", type=int, default=115200, help="Baudrate")
    parser.add_argument("--csv", default="water_measurements.csv", help="CSV đầu ra")
    parser.add_argument(
        "--interval",
        type=float,
        default=1.0,
        help="Khoảng thời gian lưu 1 mẫu (giây). Ví dụ: 1 hoặc 0.5",
    )
    return parser


def ensure_csv_header(path: Path) -> None:
    if path.exists():
        return
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                "timestamp",
                "ppm",
                "ntu",
                "distance_cm",
                "quality_code",
                "fill_request",
                "display_hold",
                "show_due",
                "pump_state",
                "pulse_us",
                "label",
                "raw",
            ]
        )


def save_row(csv_path: Path, parsed: dict) -> None:
    label = rule_label(parsed["ppm"], parsed["ntu"])

    with open(csv_path, "a", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                datetime.now().isoformat(timespec="seconds"),
                parsed["ppm"],
                parsed["ntu"],
                parsed["distance_cm"],
                parsed["quality_code"],
                parsed["fill_request"],
                parsed["display_hold"],
                parsed["show_due"],
                parsed["pump_state"],
                parsed["pulse_us"],
                label,
                parsed["raw"],
            ]
        )

    distance_text = "ERR" if parsed["distance_cm"] is None else f"{parsed['distance_cm']:.1f}"
    print(
        f"Đã lưu: PPM={parsed['ppm']:.0f}, "
        f"NTU={parsed['ntu']:.1f}, "
        f"D={distance_text}, "
        f"LABEL={label}"
    )


def main() -> None:
    args = build_argparser().parse_args()
    csv_path = Path(args.csv)
    ensure_csv_header(csv_path)

    if args.interval <= 0:
        raise ValueError("--interval phải > 0")

    latest_parsed = None
    last_save_time = time.monotonic()

    print(f"Mở cổng serial {args.port} @ {args.baud}...")
    print(f"Chu kỳ lưu mẫu: {args.interval} giây")

    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        print("Đang đọc UART và tự gắn nhãn. Nhấn Ctrl+C để dừng.\n")

        while True:
            raw_line = ser.readline().decode("utf-8", errors="ignore").strip()

            if raw_line:
                parsed = parse_uart_line(raw_line)
                if parsed:
                    latest_parsed = parsed

            now = time.monotonic()

            if latest_parsed is not None and (now - last_save_time) >= args.interval:
                save_row(csv_path, latest_parsed)
                last_save_time = now


if __name__ == "__main__":
    main()
