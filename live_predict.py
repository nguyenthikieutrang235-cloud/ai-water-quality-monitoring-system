from __future__ import annotations

import argparse
import csv
from datetime import datetime
from pathlib import Path

import joblib
import pandas as pd
import serial

from model_utils import display_label, parse_uart_line, rule_label


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Đọc UART từ STM32 và phân loại nhị phân chất lượng nước trên Raspberry Pi 5."
    )
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Cổng serial")
    parser.add_argument("--baud", type=int, default=115200, help="Baudrate")
    parser.add_argument("--model", default="water_quality_model.pkl", help="File model .pkl đã train")
    parser.add_argument("--log", default="predictions_log.csv", help="File CSV lưu kết quả dự đoán")
    parser.add_argument("--rule-only", action="store_true", help="Không dùng model, chỉ dùng rule PPM/NTU")
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
                "pump_state",
                "predicted_label",
                "raw",
            ]
        )


def predict_label(payload: dict, ppm: float, ntu: float) -> str:
    model = payload["model"]
    features = payload["features"]
    row = pd.DataFrame([[ppm, ntu]], columns=features)
    return str(model.predict(row)[0])


def main() -> None:
    args = build_argparser().parse_args()
    log_path = Path(args.log)
    ensure_csv_header(log_path)

    model_payload = None
    if not args.rule_only:
        model_path = Path(args.model)
        if not model_path.exists():
            raise FileNotFoundError(
                f"Không thấy model: {model_path}. Hãy train trước hoặc dùng --rule-only"
            )
        model_payload = joblib.load(model_path)

    print(f"Mở cổng serial {args.port} @ {args.baud}...")
    with serial.Serial(args.port, args.baud, timeout=1) as ser:
        print("Đang nhận dữ liệu. Nhấn Ctrl+C để dừng.\n")
        while True:
            raw_line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not raw_line:
                continue

            parsed = parse_uart_line(raw_line)
            if not parsed:
                print(f"Bỏ qua dòng không đúng format: {raw_line}")
                continue

            ppm = parsed["ppm"]
            ntu = parsed["ntu"]
            predicted = rule_label(ppm, ntu) if args.rule_only else predict_label(model_payload, ppm, ntu)

            shown = display_label(predicted)
            distance = "ERR" if parsed["distance_cm"] is None else f"{parsed['distance_cm']:.1f}"
            print(
                f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] "
                f"PPM={ppm:.0f} | NTU={ntu:.1f} | D={distance} | DỰ ĐOÁN={shown}"
            )
            if predicted == "ban":
                print("!!! CẢNH BÁO: Nước không đạt ngưỡng sạch !!!")

            with open(log_path, "a", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)
                writer.writerow(
                    [
                        datetime.now().isoformat(timespec="seconds"),
                        ppm,
                        ntu,
                        parsed["distance_cm"],
                        parsed["quality_code"],
                        parsed["pump_state"],
                        predicted,
                        parsed["raw"],
                    ]
                )


if __name__ == "__main__":
    main()
