#!/usr/bin/env python3
"""
极简版 ONNX 单张图片推理脚本。
修改下方常量即可运行，无需命令行参数或交互输入。
"""

import sys
from pathlib import Path

import cv2
import numpy as np
import onnxruntime as ort

# 在这里填写模型与图片路径（大写常量，运行前改成你的实际位置）
MODEL_PATH = "model/yolo12n.onnx"
IMAGE_PATH = "/Users/corn/Downloads/7552.jpg_wh860.jpg"
INPUT_SIZE = 640  # 可根据模型输入修改，若为 None 则不缩放


def load_image(path: Path, target_size: int | None) -> np.ndarray:
    """读取并预处理图片，返回 NCHW float32 张量。"""
    # 中文注释：读取 BGR 图片并转换为 RGB
    img = cv2.imread(str(path))
    if img is None:
        raise FileNotFoundError(f"无法读取图片: {path}")

    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    # 中文注释：可选 resize+letterbox 到正方形，符合 YOLO 预处理习惯
    if target_size:
        h, w = img.shape[:2]
        scale = min(target_size / h, target_size / w)
        nh, nw = int(h * scale), int(w * scale)
        resized = cv2.resize(img, (nw, nh))
        canvas = np.full((target_size, target_size, 3), 114, dtype=np.uint8)
        top = (target_size - nh) // 2
        left = (target_size - nw) // 2
        canvas[top : top + nh, left : left + nw] = resized
        img = canvas

    # 中文注释：归一化并转为 NCHW
    tensor = img.astype(np.float32) / 255.0
    tensor = np.transpose(tensor, (2, 0, 1))  # CHW
    tensor = np.expand_dims(tensor, 0)  # NCHW
    return tensor


def main():
    model_path = Path(MODEL_PATH)
    image_path = Path(IMAGE_PATH)
    target_size = INPUT_SIZE

    if not model_path.exists():
        sys.exit(f"模型不存在: {model_path}")
    if not image_path.exists():
        sys.exit(f"图片不存在: {image_path}")

    # 中文注释：最简方式加载 ONNX 模型并创建推理会话
    session = ort.InferenceSession(str(model_path))

    print("\n== ONNX 模型信息 ==")
    for i, inp in enumerate(session.get_inputs()):
        print(f"input[{i}] name={inp.name}, shape={inp.shape}, type={inp.type}")
    for i, out in enumerate(session.get_outputs()):
        print(f"output[{i}] name={out.name}, shape={out.shape}, type={out.type}")

    input_tensor = load_image(image_path, target_size)
    feed = {session.get_inputs()[0].name: input_tensor}

    print("\n== 开始推理 ==")
    outputs = session.run(None, feed)

    for idx, out in enumerate(outputs):
        arr = np.asarray(out)
        print(f"output[{idx}] shape={arr.shape}, dtype={arr.dtype}")
        flat = arr.flatten()
        preview = flat[: min(20, flat.size)]
        print(f"  preview (前 {preview.size} 项): {preview}")


if __name__ == "__main__":
    main()
