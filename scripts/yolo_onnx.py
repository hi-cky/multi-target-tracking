from numpy._typing import NDArray
import numpy as np
import cv2
import onnxruntime as ort


class Box:
    def __init__(self, x: float, y: float, w: float, h: float, type: int, prob: float):
        self.x = x
        self.y = y
        self.w = w
        self.h = h
        self.type = type
        self.prob = prob

    def __repr__(self):
        return f"Box({self.x:.2f}, {self.y:.2f}, {self.w:.2f}, {self.h:.2f}, {self.type}, {self.prob:.2f})"


class YoloOnnx:
    def __init__(
        self,
        model_path: str ,
        conf_thr: float,
        nms_iou_thr: float,
        input_width: int,
        input_height: int,
        use_gpu: bool = False,
    ):
        self.conf_thr = conf_thr
        self.nms_iou_thr = nms_iou_thr
        self.use_gpu = use_gpu

        # 根据 use_gpu 选择推理后端；若要求 GPU 但环境没有 CUDA，则直接报错
        if self.use_gpu:
            available_providers = ort.get_available_providers()
            if "CUDAExecutionProvider" not in available_providers:
                raise RuntimeError(
                    "已选择使用 GPU 推理，但当前 onnxruntime 环境不支持 CUDAExecutionProvider；"
                    "请确认安装 onnxruntime-gpu 且机器具备可用的 NVIDIA CUDA 显卡。"
                )

            # 默认使用 CUDA 的第 1 张卡（device_id=0）
            try:
                self.ort_sess = ort.InferenceSession(
                    model_path,
                    providers=[("CUDAExecutionProvider", {"device_id": 0})],
                )
            except Exception as e:
                raise RuntimeError(
                    "已选择使用 GPU 推理，但初始化 CUDA 推理会话失败；"
                    "请确认机器存在可用 CUDA 卡、驱动/CUDA 环境正常，且 device_id=0 可用。"
                ) from e
        else:
            # 默认使用 CPU 推理
            self.ort_sess = ort.InferenceSession(model_path)
        self.input_weight = input_width
        self.input_height = input_height


    def predict(
        self,
        image: NDArray,
        show: bool = False,
        window_name: str = "YOLO-ONNX",
        wait: int = 1,
    ) -> list[Box]:
        # 预处理图像
        preprocessed_image = self.preprocess(image)

        outputs = self.ort_sess.run(["output0"], {'images': preprocessed_image})

        # 输出检查（预期 outputs 是长度为 1 的 list）
        if (not isinstance(outputs, list)) or (len(outputs) != 1):
            raise ValueError("Unexpected output type")

        output = outputs[0]

        if not isinstance(output, np.ndarray):
            raise ValueError("Unexpected output type")

        # Postprocess outputs
        predict_boxes = self.nms(output[0].transpose((1, 0)))

        # 如需可视化，则把检测框映射回原图坐标后画出来
        if show:
            vis_boxes = self._map_boxes_to_original(image, predict_boxes)
            self.visualize(image, vis_boxes, window_name=window_name, wait=wait)

        return predict_boxes


    def preprocess(self, image: NDArray) -> NDArray:
        # Get image dimensions from numpy array
        img_h, img_w = image.shape[:2]

        # Letterbox
        ratio = min(self.input_height / img_h, self.input_weight / img_w)
        new_width = int(img_w * ratio)
        new_height = int(img_h * ratio)

        # Resize using OpenCV (assumes image is BGR format)
        resized = cv2.resize(image, (new_width, new_height), interpolation=cv2.INTER_LANCZOS4)

        # Create a black canvas
        canvas = np.zeros((self.input_height, self.input_weight, 3), dtype=np.uint8)

        # Calculate paste coordinates
        x_offset = (self.input_weight - new_width) // 2
        y_offset = (self.input_height - new_height) // 2

        # Paste resized image onto canvas
        canvas[y_offset:y_offset+new_height, x_offset:x_offset+new_width] = resized

        # Normalize image to [0, 1] range
        image_np = canvas.astype(np.float32) / 255.0

        # Transpose to (C, H, W) and add batch dimension
        image_np = image_np.transpose((2, 0, 1)).astype(np.float32)
        image_np = np.expand_dims(image_np, axis=0)

        return image_np


    def nms(self, results: NDArray) -> list[Box]:
        boxes: list[Box] = []
        for result in results:
            box = result[:4]
            probs = result[4:]
            type = np.argmax(probs)
            prob = probs[type]
            if type != 0 or prob < self.conf_thr:
                continue

            boxes.append(Box(
                x=box[0] - box[2] / 2,
                y=box[1] - box[3] / 2,
                w=box[2],
                h=box[3],
                type=int(type),
                prob=prob
            ))

        # sort by prob
        sorted_boxes = sorted(boxes, key=lambda box: box.prob, reverse=True)
        selected_boxes: list[Box] = []
        for box in sorted_boxes:
            if not any([iou(box, selected_box) > self.nms_iou_thr for selected_box in selected_boxes]):
                selected_boxes.append(box)

        return selected_boxes

    def _map_boxes_to_original(self, image: NDArray, boxes: list[Box]) -> list[Box]:
        # 把 letterbox 输入空间的框，映射回原图空间（用于可视化）
        img_h, img_w = image.shape[:2]
        ratio = min(self.input_height / img_h, self.input_weight / img_w)
        new_width = int(img_w * ratio)
        new_height = int(img_h * ratio)
        x_offset = (self.input_weight - new_width) // 2
        y_offset = (self.input_height - new_height) // 2

        mapped: list[Box] = []
        for b in boxes:
            # 这里沿用当前工程的 Box 语义（x,y,w,h 视为左上角+宽高）
            x = (b.x - x_offset) / ratio
            y = (b.y - y_offset) / ratio
            w = b.w / ratio
            h = b.h / ratio

            # 裁剪到图像范围内，避免画框越界
            x = float(np.clip(x, 0, max(0, img_w - 1)))
            y = float(np.clip(y, 0, max(0, img_h - 1)))
            w = float(np.clip(w, 0, img_w - x))
            h = float(np.clip(h, 0, img_h - y))

            mapped.append(Box(x=x, y=y, w=w, h=h, type=b.type, prob=float(b.prob)))

        return mapped

    def visualize(
        self,
        image: NDArray,
        boxes: list[Box],
        window_name: str = "YOLO-ONNX",
        wait: int = 1,
    ) -> NDArray:
        # 使用 OpenCV 在当前帧上绘制检测框并展示（返回绘制后的图像）
        vis = image.copy()
        for b in boxes:
            x1, y1 = int(round(b.x)), int(round(b.y))
            x2, y2 = int(round(b.x + b.w)), int(round(b.y + b.h))

            cv2.rectangle(vis, (x1, y1), (x2, y2), (0, 255, 0), 2)
            label = f"id={b.type} {b.prob:.2f}"
            cv2.putText(
                vis,
                label,
                (x1, max(0, y1 - 5)),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6,
                (0, 255, 0),
                2,
                cv2.LINE_AA,
            )

        cv2.imshow(window_name, vis)
        cv2.waitKey(wait)
        return vis


def iou(box1: Box, box2: Box) -> float:
    area1, area2 = box1.w * box1.h, box2.w * box2.h
    x5 = max(box1.x, box2.x)
    y5 = max(box1.y, box2.y)
    x6 = min(box1.x + box1.w, box2.x + box2.w)
    y6 = min(box1.y + box1.h, box2.y + box2.h)

    w3, h3 = x6 - x5, y6 - y5
    area3 = w3 * h3 if w3 > 0 and h3 > 0 else 0
    return area3 / (area1 + area2 - area3)
