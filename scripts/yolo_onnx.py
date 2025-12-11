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
        input_height: int
    ):
        self.conf_thr = conf_thr
        self.nms_iou_thr = nms_iou_thr
        self.ort_sess = ort.InferenceSession(model_path)
        self.input_weight = input_width
        self.input_height = input_height
        
        
    def predict(self, image: NDArray) -> list[Box]:
        # Preprocess image
        preprocessed_image = self.preprocess(image)
        
        outputs = self.ort_sess.run(["output0"], {'images': preprocessed_image})
        
        if not isinstance(outputs, list) and len(outputs) != 1:
            raise ValueError("Unexpected output type")
            
        output = outputs[0]
        
        if not isinstance(output, np.ndarray):
            raise ValueError("Unexpected output type")
        
        # Postprocess outputs
        predict_boxes = self.nms(output[0].transpose((1, 0)))
        
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
                x=box[0],
                y=box[1],
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
        
        
def iou(box1: Box, box2: Box) -> float:
    area1, area2 = box1.w * box1.h, box2.w * box2.h
    x5 = max(box1.x, box2.x)
    y5 = max(box1.y, box2.y)
    x6 = min(box1.x + box1.w, box2.x + box2.w)
    y6 = min(box1.y + box1.h, box2.y + box2.h)
    
    w3, h3 = x6 - x5, y6 - y5
    area3 = w3 * h3 if w3 > 0 and h3 > 0 else 0
    return area3 / (area1 + area2 - area3)