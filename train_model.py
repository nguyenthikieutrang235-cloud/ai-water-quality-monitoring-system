from __future__ import annotations

import argparse
import json
from pathlib import Path

import joblib
import pandas as pd
from sklearn.metrics import accuracy_score, classification_report
from sklearn.model_selection import train_test_split
from sklearn.tree import DecisionTreeClassifier

from model_utils import rule_label

FEATURES = ["ppm", "ntu"]


def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Train model phân loại nhị phân chất lượng nước từ PPM và NTU."
    )
    parser.add_argument(
        "--csv",
        default="water_measurements.csv",
        help="Đường dẫn file CSV có cột ppm, ntu. Nhãn cũ sẽ bị ghi đè theo rule mới.",
    )
    parser.add_argument(
        "--model-out",
        default="water_quality_model.pkl",
        help="File model đầu ra",
    )
    parser.add_argument(
        "--metrics-out",
        default="training_metrics.json",
        help="File JSON lưu kết quả train",
    )
    return parser


def main() -> None:
    args = build_argparser().parse_args()
    csv_path = Path(args.csv)

    if not csv_path.exists():
        raise FileNotFoundError(f"Không tìm thấy file CSV: {csv_path}")

    df = pd.read_csv(csv_path)

    required_cols = {"ppm", "ntu"}
    if not required_cols.issubset(df.columns):
        raise ValueError("CSV phải có ít nhất 2 cột: ppm, ntu")

    df = df.copy()
    df["ppm"] = pd.to_numeric(df["ppm"], errors="coerce")
    df["ntu"] = pd.to_numeric(df["ntu"], errors="coerce")
    df = df.dropna(subset=["ppm", "ntu"]).copy()

    # Luôn gắn lại nhãn theo rule mới để không bị lẫn nhãn cũ 3 lớp
    df["label"] = [rule_label(ppm, ntu) for ppm, ntu in zip(df["ppm"], df["ntu"])]

    if len(df) < 6:
        raise ValueError("Cần ít nhất 6 dòng dữ liệu để train thử.")

    X = df[FEATURES]
    y = df["label"]

    class_counts = y.value_counts()
    can_split = len(class_counts) >= 2 and class_counts.min() >= 2 and len(df) >= 12

    model = DecisionTreeClassifier(
        max_depth=3,
        random_state=42,
        class_weight="balanced",
    )

    metrics: dict[str, object] = {
        "dataset_rows": int(len(df)),
        "class_counts": {k: int(v) for k, v in class_counts.to_dict().items()},
        "features": FEATURES,
        "model": "DecisionTreeClassifier(max_depth=3)",
        "labeling_rule": {
            "sach": "PPM <= 300 và NTU <= 220",
            "ban": "mọi trường hợp còn lại",
        },
    }

    if can_split:
        X_train, X_test, y_train, y_test = train_test_split(
            X,
            y,
            test_size=0.25,
            random_state=42,
            stratify=y,
        )
        model.fit(X_train, y_train)
        y_pred = model.predict(X_test)

        metrics["train_rows"] = int(len(X_train))
        metrics["test_rows"] = int(len(X_test))
        metrics["accuracy"] = float(accuracy_score(y_test, y_pred))
        metrics["classification_report"] = classification_report(
            y_test,
            y_pred,
            output_dict=True,
            zero_division=0,
        )
    else:
        model.fit(X, y)
        metrics["note"] = (
            "Dữ liệu còn ít hoặc chỉ có một lớp nên model được fit trên toàn bộ dữ liệu, "
            "chưa tách test."
        )

    payload = {
        "model": model,
        "features": FEATURES,
        "labels": sorted(y.unique().tolist()),
        "labeling_rule": metrics["labeling_rule"],
    }
    joblib.dump(payload, args.model_out)

    with open(args.metrics_out, "w", encoding="utf-8") as f:
        json.dump(metrics, f, ensure_ascii=False, indent=2)

    print("Đã train xong.")
    print(f"- Model:   {Path(args.model_out).resolve()}")
    print(f"- Metrics: {Path(args.metrics_out).resolve()}")
    if "accuracy" in metrics:
        print(f"- Accuracy test: {metrics['accuracy']:.4f}")
    else:
        print(f"- Ghi chú: {metrics['note']}")


if __name__ == "__main__":
    main()
