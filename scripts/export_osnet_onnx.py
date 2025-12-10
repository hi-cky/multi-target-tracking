# #!/usr/bin/env python3
# """下载并导出 OSNet 行人特征提取模型到 ONNX。"""

# from pathlib import Path
# import argparse
# import torch
# import torch.nn.functional as F
# from torch import nn
# from torchreid.reid import models
# from torchreid.reid.utils import load_pretrained_weights


# class OSNetWrapper(nn.Module):
#     """仅输出 L2 归一化后的特征向量，方便 C++ 侧直接用。"""

#     def __init__(self, backbone: nn.Module):
#         super().__init__()
#         self.backbone = backbone.eval()

#     def forward(self, x: torch.Tensor) -> torch.Tensor:
#         with torch.no_grad():
#             feats = self.backbone(x)  # (N, C)
#             feats = F.normalize(feats, p=2, dim=1)
#             return feats


# def parse_args():
#     parser = argparse.ArgumentParser(description="导出 OSNet ONNX")
#     parser.add_argument("--output", type=Path, default=Path("model/osnet_x1_0.onnx"),
#                         help="过程级别中文注释：导出的 ONNX 路径")
#     parser.add_argument("--height", type=int, default=256, help="输入高度")
#     parser.add_argument("--width", type=int, default=128, help="输入宽度")
#     parser.add_argument("--opset", type=int, default=12, help="ONNX opset 版本")
#     return parser.parse_args()


# def main():
#     args = parse_args()
#     repo_root = Path(__file__).resolve().parents[1]
#     out_path = (repo_root / args.output).resolve()

#     # 过程级别中文注释：构建 OSNet 并加载官方预训练权重
#     backbone = models.osnet_x1_0(num_classes=1000, loss="softmax", pretrained=True)
#     wrapper = OSNetWrapper(backbone).eval()

#     dummy = torch.randn(1, 3, args.height, args.width)

#     out_path.parent.mkdir(parents=True, exist_ok=True)
#     torch.onnx.export(
#         wrapper,
#         dummy,
#         str(out_path),
#         export_params=True,
#         opset_version=args.opset,
#         do_constant_folding=True,
#         input_names=["images"],
#         output_names=["features"],
#         dynamic_axes={"images": {0: "batch"}, "features": {0: "batch"}},
#     )
#     print(f"已导出 ONNX -> {out_path}")


# if __name__ == "__main__":
#     main()
