# %%
from ast import Tuple
import os
print(os.system("pwd"))

# %%
# scripts/test_pretty.py
import onnx
MODEL_PATH = "../model/yolo12n.onnx"
onnx_model = onnx.load(MODEL_PATH)
print("IR 版本:", onnx_model.ir_version)
print("算子域:", [(op.domain or "ai.onnx", op.version) for op in onnx_model.opset_import])

# 使用 helper 生成更易读的图结构
graph_txt = onnx.printer.to_text(onnx_model.graph)
print(graph_txt[:200])  # 只看前 200 字符，避免信息过载
# %%
import onnxruntime as ort
import numpy as np
from numpy._core.multiarray import ndarray
from PIL import Image

HEIGHT, WEIGHT = (640, 640)
def yolo_preprocess(image_path, height=640, width=640):
    image = Image.open(image_path)

    # Letterbox
    ratio = min(height / image.height, width / image.width)
    new_width = int(image.width * ratio)
    new_height = int(image.height * ratio)
    image = image.resize((new_width, new_height), Image.Resampling.LANCZOS)

    # Create a black canvas
    canvas = Image.new('RGB', (width, height), (0, 0, 0))
    canvas.paste(image, ((width - new_width) // 2, (height - new_height) // 2))
    
    import matplotlib.pyplot as plt
    
    plt.imshow(canvas)

    # Normalize image to [0, 1] range
    image_np = np.array(canvas).astype(np.float32) / 255.0

    # Transpose to (C, H, W) and add batch dimension
    image_np = image_np.transpose((2, 0, 1)).astype(np.float32)
    image_np = np.expand_dims(image_np, axis=0)

    return image_np, canvas
    
IMAGE_PATH = "/Users/corn/Downloads/7552.jpg_wh860.jpg"

image, canvas = yolo_preprocess(IMAGE_PATH)

# 把canvas转成cv2图像
canvas = np.array(canvas)

# %%

ort_sess = ort.InferenceSession(MODEL_PATH)

outputs = ort_sess.run(["output0"], {'images': image})

# Print Result
if isinstance(outputs, list):
    for output in outputs:
        output = output
        if not isinstance(output, ndarray):
            continue
        print(output.shape)
        result = output
else:
    print(type(outputs))
# %% 直观感受一下输出的每个框
PROB_THR = 0.25

results = output[0].transpose((1, 0))
print(results.shape)

for result in results:
    box = result[:4]
    probs = result[4:]
    type = np.argmax(probs)
    prob = probs[type]
    if type != 0 or prob < PROB_THR:
        continue
    print(f"Box: {box}")
    print(f"Class: {type} Prob: {prob:.2f}")

# %% 真正的 nms
IOU_THR = 0.7

def iou(box1, box2):
    x1, y1, w1, h1 = box1
    x2, y2, w2, h2 = box2
    area1, area2 = w1 * h1, w2 * h2
    x5 = max(x1, x2)
    y5 = max(y1, y2)
    x6 = min(x1 + w1, x2 + w2)
    y6 = min(y1 + h1, y2 + h2)
    
    w3, h3 = x6 - x5, y6 - y5
    area3 = w3 * h3 if w3 > 0 and h3 > 0 else 0
    return area3 / (area1 + area2 - area3)

def nms(results: np.ndarray) -> ndarray:
    boxes = []
    for result in results:
        box = result[:4]
        probs = result[4:]
        type = np.argmax(probs)
        prob = probs[type]
        if type != 0 or prob < PROB_THR:
            continue
            
        boxes.append(np.concatenate([box, [type, prob]]))
    boxes = np.array(boxes)
    print(boxes.shape)
    
    # sort by prob
    sorted_boxes_indice = np.argsort(boxes[:, -1], axis=0)[::-1]
    sorted_boxes = boxes[sorted_boxes_indice]
    selected_boxes = []
    for box in sorted_boxes:
        if not any([iou(box[:4], selected_box[:4]) > IOU_THR for selected_box in selected_boxes]):
            selected_boxes.append(box)
            
    return np.array(selected_boxes)
    
print(nms(results))
# %% draw result
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches

def draw_result(image, results):
    fig, ax = plt.subplots(figsize=(10, 10))
    ax.imshow(image)

    for det in results:
        cx1, cy1, w, h, cls_id, score = det

        x1 = cx1 - w / 2
        y1 = cy1 - h / 2

        # 使用 matplotlib.patches.Rectangle 画框
        rect = patches.Rectangle(
            (x1, y1),
            w,
            h,
            linewidth=2,
            edgecolor='red',
            facecolor='none'
        )
        ax.add_patch(rect)

        # 在框上方写字
        ax.text(
            x1,
            y1 - 5,
            f"{int(cls_id)} {score:.2f}",
            color="yellow",
            fontsize=12,
            bbox=dict(facecolor="red", alpha=0.5)
        )

    plt.axis("off")
    plt.show()
draw_result(canvas, nms(results))
