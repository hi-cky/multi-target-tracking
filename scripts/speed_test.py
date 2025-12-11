from matplotlib.cbook import boxplot_stats
from yolo_onnx import YoloOnnx, Box
from osnet_onnx import OsnetOnnx
from typing import Iterator
import cv2
import numpy as np
from typing import Iterator
from line_profiler import profile


def video_frame_iterator(video_path: str, fps: int = 30) -> Iterator[np.ndarray]:
    """
    视频帧迭代器

    Args:
        video_path: 视频文件路径
        fps: 采样帧率，默认30帧/秒

    Yields:
        numpy格式的图片帧
    """
    cap = cv2.VideoCapture(video_path)

    if not cap.isOpened():
        raise ValueError(f"无法打开视频文件: {video_path}")

    # 获取视频原始帧率
    original_fps = cap.get(cv2.CAP_PROP_FPS)
    if original_fps <= 0:
        original_fps = fps

    # 计算采样间隔（每隔多少帧取一帧）
    frame_interval = max(1, int(original_fps / fps))

    frame_count = 0
    while True:
        ret, frame = cap.read()
        if not ret:
            break

        # 根据采样间隔决定是否返回当前帧
        if frame_count % frame_interval == 0:
            yield frame

        frame_count += 1

    cap.release()

@profile
def speed_test(
    video_iterator: Iterator[np.ndarray],
    yolo: YoloOnnx,
    osnet: OsnetOnnx
):
    """
    测试速度

    Args:
        video_path: 视频文件路径
    """

    for frame in video_iterator:
        boxes: list[Box] = yolo.predict(frame)
        for box in boxes:
            # 裁切出对应 Box 的图片
            cropped_image = frame[
                int(box.y):int(box.y+box.h), 
                int(box.x):int(box.x+box.w)
            ]
            osnet.predict(cropped_image)


if __name__ == "__main__":
    video_iterator = video_frame_iterator(
        "/Users/corn/Movies/30632380830-1-192.mp4",
        fps=1
    )
    yolo = YoloOnnx(
        model_path="../model/yolo12n.onnx",
        conf_thr=0.25,
        nms_iou_thr=0.7,
        input_width=640,
        input_height=640
    )
    osnet = OsnetOnnx(
        "../model/osnet_x1_0.onnx",
        input_width=128,
        input_height=256
    )
    
    speed_test(video_iterator, yolo, osnet)