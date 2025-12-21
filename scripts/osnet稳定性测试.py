from osnet_onnx import OsnetOnnx
from yolo_onnx import YoloOnnx, Box
from speed_test import video_frame_iterator
import numpy as np
import time


def _l2_normalize(x: np.ndarray, eps: float = 1e-12) -> np.ndarray:
    # 对向量做 L2 归一化（避免余弦相似度/统计量受到尺度影响）
    norm = np.linalg.norm(x, axis=-1, keepdims=True)
    return x / np.maximum(norm, eps)


def evaluate_embedding_stability(embeddings: np.ndarray) -> None:
    # 给一串 embedding（形状 N×D）输出几个更直观的“稳定性”标量指标
    # - 相邻帧余弦相似度：越接近 1 越稳定（可以抓到瞬时跳变）
    # - 到中心向量的余弦相似度：越集中越稳定（整体漂移/抖动）
    # - 方差的汇总：把“每一维方差向量”压缩成几个易读数字
    if embeddings.ndim != 2:
        raise ValueError(f"embeddings 期望是二维数组(N, D)，但得到 {embeddings.shape}")

    n, d = embeddings.shape
    if n < 2:
        print("样本数不足（至少需要 2 个 embedding 才能评估稳定性）")
        return

    # 统一做一次归一化，避免某些情况下 predict 没有严格归一化
    emb = _l2_normalize(embeddings.astype(np.float32))

    # 1) 相邻帧余弦相似度（embedding 已归一化时，点积就是余弦）
    adj_cos = np.sum(emb[:-1] * emb[1:], axis=1)  # (N-1,)

    # 2) 到“中心向量”的余弦相似度（中心=均值后再归一化）
    center = _l2_normalize(np.mean(emb, axis=0, keepdims=True))[0]  # (D,)
    center_cos = emb @ center  # (N,)

    # 3) 每一维的方差（仍是 D 维），但我们把它汇总成几个标量
    per_dim_var = np.var(emb, axis=0)  # (D,)
    per_dim_std = np.sqrt(per_dim_var)

    mean_var = float(np.mean(per_dim_var))
    rms_std = float(np.sqrt(np.mean(per_dim_var)))  # 等价于 per_dim_std 的 RMS
    p95_std = float(np.percentile(per_dim_std, 95))

    # 4) 额外：找出抖动最大的几个维度（方便排查，但一般不必深究）
    topk = min(10, d)
    top_idx = np.argsort(-per_dim_std)[:topk]

    # 打印摘要（尽量用少量数字把“稳不稳”说明白）
    print("\n========== Embedding 稳定性评估 ==========")
    print(f"样本数 N={n}, 维度 D={d}")

    print("\n[相邻帧余弦相似度]（越接近 1 越稳定）🙂")
    print(f"  mean={float(np.mean(adj_cos)):.6f}")
    print(f"  std ={float(np.std(adj_cos)):.6f}")
    print(f"  min ={float(np.min(adj_cos)):.6f}")
    print(f"  p05 ={float(np.percentile(adj_cos, 5)):.6f}")

    print("\n[到中心向量余弦相似度]（越接近 1 且波动越小越稳定）📌")
    print(f"  mean={float(np.mean(center_cos)):.6f}")
    print(f"  std ={float(np.std(center_cos)):.6f}")
    print(f"  min ={float(np.min(center_cos)):.6f}")
    print(f"  p05 ={float(np.percentile(center_cos, 5)):.6f}")

    print("\n[方差汇总]（把 D 维方差压缩成标量，更好读）🧾")
    print(f"  mean_var={mean_var:.8f}")
    print(f"  rms_std ={rms_std:.8f}")
    print(f"  p95_std ={p95_std:.8f}")

    print("\n[抖动最大的维度 Top-K]（一般仅用于排查）🔍")
    for rank, idx in enumerate(top_idx, start=1):
        print(f"  #{rank:02d} dim={int(idx):4d} std={float(per_dim_std[idx]):.8f}")


def evaluate_multi_person_discriminability(embeddings_by_frame: list[np.ndarray]) -> None:
    # 评估“同一帧不同人的特征区分度”
    # 思路：对每帧的多个人 embedding（形状 M×D，且每行已 L2 归一化）计算两两余弦相似度；
    # - 相似度越低，区分度越好
    # - 每帧的“最大相似度”很关键：一旦很高，说明存在两个人特征很像/框错了/同一个人被重复框
    pairwise_cos_all: list[float] = []
    per_frame_max: list[float] = []
    per_frame_mean: list[float] = []
    frames_used = 0

    for emb in embeddings_by_frame:
        if emb.ndim != 2:
            continue
        n, _ = emb.shape
        if n < 2:
            continue

        frames_used += 1
        e = _l2_normalize(emb.astype(np.float32))

        # 余弦相似度矩阵 = E @ E^T（对角线是 1）
        sim = e @ e.T  # (n, n)
        mask = ~np.eye(n, dtype=bool)
        sims = sim[mask]  # (n*n-n,)

        pairwise_cos_all.extend([float(x) for x in sims])
        per_frame_max.append(float(np.max(sims)))
        per_frame_mean.append(float(np.mean(sims)))

    print("\n========== 多人区分度评估（同帧不同 Box） ==========")
    if frames_used == 0 or len(pairwise_cos_all) == 0:
        print("没有足够的帧包含 2 个及以上目标，无法评估多人区分度。")
        return

    sims_all = np.array(pairwise_cos_all, dtype=np.float32)
    pf_max = np.array(per_frame_max, dtype=np.float32)
    pf_mean = np.array(per_frame_mean, dtype=np.float32)

    print(f"参与统计的帧数={frames_used}，两两对比总数={sims_all.size}")

    print("\n[两两余弦相似度总体分布]（越低越好）🧑‍🤝‍🧑")
    print(f"  mean={float(np.mean(sims_all)):.6f}")
    print(f"  std ={float(np.std(sims_all)):.6f}")
    print(f"  min ={float(np.min(sims_all)):.6f}")
    print(f"  p50 ={float(np.percentile(sims_all, 50)):.6f}")
    print(f"  p95 ={float(np.percentile(sims_all, 95)):.6f}")
    print(f"  max ={float(np.max(sims_all)):.6f}")

    print("\n[每帧最大相似度]（抓最容易混淆的那一对）⚠️")
    print(f"  mean={float(np.mean(pf_max)):.6f}")
    print(f"  p95 ={float(np.percentile(pf_max, 95)):.6f}")
    print(f"  max ={float(np.max(pf_max)):.6f}")

    print("\n[每帧平均相似度]（整体区分度概览）📌")
    print(f"  mean={float(np.mean(pf_mean)):.6f}")
    print(f"  p95 ={float(np.percentile(pf_mean, 95)):.6f}")


def main():
    video_iterator = video_frame_iterator(
        "/home/corn/share/体操.mp4",
        fps=1
    )
    yolo = YoloOnnx(
        model_path="model/yolo12n.onnx",
        conf_thr=0.25,
        nms_iou_thr=0.7,
        input_width=640,
        input_height=640,
        use_gpu=True
    )
    osnet = OsnetOnnx(
        "model/osnet_x1_0.onnx",
        input_width=128,
        input_height=256,
        use_gpu=True
    )
    # 单人稳定性：每帧取第一个有效框收集 embedding（和之前逻辑保持一致）
    embedding_list: list[np.ndarray] = []
    # 多人区分度：按帧收集“同帧多框”的 embeddings（每帧一个 (M, D) 数组）
    embeddings_by_frame: list[np.ndarray] = []
    # 为了避免同帧框太多导致 O(M^2) 计算太重，这里限制每帧最多取多少个框
    max_people_per_frame = 6

    for frame in video_iterator:
        boxes: list[Box] = yolo.predict(frame, show=True)
        if not boxes:
            # 当前帧没有检出目标，跳过
            continue

        # 对同一帧的多个框提特征（用于“多人区分度”）
        h, w = frame.shape[:2]
        frame_embeddings: list[np.ndarray] = []
        first_embedding: np.ndarray | None = None
        for i, box in enumerate(boxes[:max_people_per_frame]):
            # 裁切出对应 Box 的图片（做一次边界裁剪，避免越界导致空图）
            x1 = int(max(0, min(w - 1, box.x)))
            y1 = int(max(0, min(h - 1, box.y)))
            x2 = int(max(0, min(w, box.x + box.w)))
            y2 = int(max(0, min(h, box.y + box.h)))
            if x2 <= x1 or y2 <= y1:
                continue

            cropped_image = frame[y1:y2, x1:x2]
            emb = osnet.predict(cropped_image)
            frame_embeddings.append(emb)

            # 单人稳定性：取本帧第一个有效框的 embedding
            if first_embedding is None:
                first_embedding = emb

        if first_embedding is not None:
            embedding_list.append(first_embedding)

        if len(frame_embeddings) >= 2:
            embeddings_by_frame.append(np.array(frame_embeddings, dtype=np.float32))

    # 把收集到的 embedding 变成 (N, D) 的数组，并评估稳定性
    embedding_array = np.array(embedding_list, dtype=np.float32)
    if embedding_array.size == 0:
        print("没有收集到任何 embedding（可能一直没检测到目标）")
        return

    evaluate_embedding_stability(embedding_array)
    evaluate_multi_person_discriminability(embeddings_by_frame)



if __name__ == "__main__":
    main()
