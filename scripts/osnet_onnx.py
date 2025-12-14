from numpy._typing import NDArray
import numpy as np
import cv2
import onnxruntime as ort


class OsnetOnnx:
    def __init__(
        self,
        model_path: str,
        input_width: int,
        input_height: int,
        use_gpu: bool = False,
    ):
        # 初始化 ONNX Runtime 会话并记录输入尺寸
        self.input_width = input_width
        self.input_height = input_height
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
                # CUDA provider 可用但创建会话失败，通常是机器无可用 CUDA 卡/驱动问题
                raise RuntimeError(
                    "已选择使用 GPU 推理，但初始化 CUDA 推理会话失败；"
                    "请确认机器存在可用 CUDA 卡、驱动/CUDA 环境正常，且 device_id=0 可用。"
                ) from e
        else:
            # 默认使用 CPU 推理
            self.ort_sess = ort.InferenceSession(model_path)

        # 记住模型的输入/输出名，避免每次推理查询
        self._input_name = self.ort_sess.get_inputs()[0].name
        self._output_name = self.ort_sess.get_outputs()[0].name

    def predict(self, image: NDArray) -> NDArray:
        """对单张图像提取特征，返回 L2 归一化后的 1D 向量。"""
        if image is None or image.size == 0:
            raise ValueError("输入图像为空")

        # 预处理 -> NCHW float32
        input_tensor = self.preprocess(image)

        outputs = self.ort_sess.run([self._output_name], {self._input_name: input_tensor})

        if not outputs:
            raise ValueError("ONNX 推理输出为空")

        feat = outputs[0]
        if not isinstance(feat, np.ndarray):
            raise ValueError("ONNX 输出类型异常")

        # 去掉 batch 维度并做 L2 归一化
        feat = np.squeeze(feat)
        norm = np.linalg.norm(feat)
        if norm > 0:
            feat = feat / norm

        return feat

    def preprocess(self, image: NDArray) -> NDArray:
        # 按模型输入大小缩放，并从 BGR 转 RGB
        resized = cv2.resize(image, (self.input_width, self.input_height))
        resized = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)

        # 归一化到 [0,1]，再做 ImageNet 均值方差标准化
        img = resized.astype(np.float32) / 255.0
        mean = np.array([0.485, 0.456, 0.406], dtype=np.float32)
        std = np.array([0.229, 0.224, 0.225], dtype=np.float32)
        img = (img - mean) / std

        # HWC -> CHW，并添加 batch 维
        img = img.transpose((2, 0, 1))
        img = np.expand_dims(img, axis=0)

        return img
