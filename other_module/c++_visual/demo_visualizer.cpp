#include <exception>
#include <iostream>

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "AppendixA.h"
#include "Visualizer.h"

int main()
{
    try
    {
        const int w = 960;
        const int h = 540;
        cv::Mat frame(h, w, CV_8UC3, cv::Scalar(25, 25, 25));

        LabeledFrame labeled;
        labeled.frame_index = 0;
        labeled.objs.push_back(LabeledObject{1, cv::Rect(60, 80, 240, 360), 0, 0.92f});
        labeled.objs.push_back(LabeledObject{2, cv::Rect(360, 140, 220, 260), 0, 0.85f});
        labeled.objs.push_back(LabeledObject{42, cv::Rect(620, 110, 260, 320), 0, 0.78f});

        Visualizer::Options opts;
        opts.showScore = true;
        Visualizer viz(opts);

        const cv::Mat out = viz.render(frame, labeled);

        const std::string outPath = "demo_visualizer_output.png";
        if (!cv::imwrite(outPath, out))
        {
            std::cerr << "Failed to write " << outPath << "\n";
            return 2;
        }

        std::cout << "Wrote " << outPath << "\n";

        try
        {
            cv::imshow("Visualizer Demo", out);
            cv::waitKey(0);
        }
        catch (const cv::Exception &e)
        {
            std::cerr << "OpenCV GUI error: " << e.what() << "\n";
            std::cerr << "Saved image to " << outPath << " (GUI display skipped).\n";
        }

        return 0;
    }
    catch (const cv::Exception &e)
    {
        std::cerr << "OpenCV error: " << e.what() << "\n";
        std::cerr << "Tip: if this fails before opening a window, you may be missing runtime DLLs; try running `run_demo_visualizer.ps1`.\n";
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
