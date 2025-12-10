#!/usr/bin/env python3
"""将 YOLOv12 PyTorch 权重导出为 ONNX 文件，方便 C++ 推理。"""

import argparse
from pathlib import Path

from ultralytics.models import YOLO

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="导出 YOLOv12n ONNX")
    parser.add_argument(
        "--weights",
        type=Path,
        default=Path("model/yolo12n.pt"),
        help="过程级别中文注释：指定待转换的 .pt 权重路径",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("model/yolo12n.onnx"),
        help="过程级别中文注释：指定导出的 ONNX 保存位置",
    )
    parser.add_argument(
        "--imgsz",
        type=int,
        default=640,
        help="过程级别中文注释：推理输入尺寸，需与 C++ 推理保持一致",
    )
    parser.add_argument(
        "--opset",
        type=int,
        default=12,
        help="过程级别中文注释：ONNX opset 版本，需兼容 onnxruntime",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    weights_path = (repo_root / args.weights).resolve()
    output_path = (repo_root / args.output).resolve()

    if not weights_path.exists():
        raise FileNotFoundError(f"未找到权重文件: {weights_path}")

    # 过程级别中文注释：加载 YOLOv12n PyTorch 权重
    model = YOLO(str(weights_path))

    # 过程级别中文注释：通过官方 export API 直接生成 ONNX
    exported_path = model.export(
        format="onnx",
        imgsz=args.imgsz,
        opset=args.opset,
        simplify=True,
        dynamic=True,
    )

    exported = Path(exported_path)
    if exported.resolve() != output_path:
        output_path.write_bytes(exported.read_bytes())

    print(f"已成功将 {weights_path.name} 转为 {output_path.name}")


if __name__ == "__main__":
    main()
