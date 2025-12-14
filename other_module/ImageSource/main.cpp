#include "ImageSource.h"
#include <iostream>
#include <fstream> // 添加此头文件以修复 std::ifstream 不完整类型错误

// 替换为您的视频文件路径
#define TEST_VIDEO_PATH "C:\\CExamples\\videocapture\\7e29021ad593bd0afed25c804062c2e9.mp4"

int main() {
    std::cout << "--- 视频采集模块自测 ---" << std::endl;
    IImageIterator* iterator = nullptr;

    try {
		// 1. 创建视频源,无法打开文件会抛出异常
        std::cout << "尝试打开视频源: " << TEST_VIDEO_PATH << std::endl;
        if (!std::ifstream(TEST_VIDEO_PATH)) {
            throw std::runtime_error("指定的视频文件不存在或无法访问。请检查路径是否正确。");
		}
        VideoFrameSource source(TEST_VIDEO_PATH);

        // 2. 创建迭代器
        iterator = source.createIterator();
        std::cout << "视频源创建成功，迭代器已准备。" << std::endl;

        cv::Mat frame;
        int frame_count = 0;
        int max_frames_to_show = 5; // 仅显示前5帧作为演示

        // 3. 逐帧读取并显示图像
        while (iterator->hasNext() && frame_count < max_frames_to_show) {
            if (iterator->next(frame)) {
                frame_count++;
                std::cout << "成功读取第 " << frame_count << " 帧。" << std::endl;
                // 文件将输出到程序运行目录下的 framestorage 文件夹
                std::string filename = "framestorage/output_frame_" + std::to_string(frame_count) + ".jpg";
                cv::imwrite(filename, frame);
				//检测是否成功保存图像
                if (!frame.empty()) {
                    std::cout << "第 " << frame_count << " 帧已保存为图像文件。" << std::endl;
                } else {
                    std::cout << "第 " << frame_count << " 帧保存失败，图像为空。" << std::endl;
				}
                // 使用 cv::imshow 显示图像 [cite: 17]
                cv::imshow("Video Frame", frame);

                // 等待 30 毫秒，以便能看到图像。按任意键继续。
                if (cv::waitKey(90) >= 0) {
                    break;
                }
            }
            else {
                std::cout << "迭代器返回 false，视频读取完毕或失败。" << std::endl;
                break;
            }
        }

        std::cout << "总共读取并显示了 " << frame_count << " 帧。" << std::endl;

    }
    catch (const std::runtime_error& e) {
        // 捕获构造函数或 next() 中抛出的异常 [cite: 9, 29]
        std::cerr << "!!! 错误: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "!!! 发生未知错误: " << e.what() << std::endl;
        return 1;
    }

    // 4. 清理资源 (重要：释放动态分配的迭代器)
    if (iterator) {
        delete iterator;
        iterator = nullptr;
    }

    // 关闭所有 OpenCV 窗口
    cv::destroyAllWindows();

    std::cout << "--- 测试结束 ---" << std::endl;
    return 0;
}