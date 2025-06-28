#include <opencv2/opencv.hpp>
#include <opencv2/photo.hpp>
#include <iostream>
#include <string>
#include <algorithm>

using namespace cv;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

static void usage(const char* p) {
    cerr << "Usage:\n  " << p
         << " <in.jpg> <x> <y> <width> <height> [--margin p] [--alpha] [--scale s] [out.png]\n\n"
            "  x, y, width, height  ROI in original image coords (mandatory)\n"
            "  --margin p           margin percentage around ROI (0–100, default 0)\n"
            "  --alpha              output RGBA PNG with transparent background\n"
            "  --scale s            downscale factor (0.1–1.0, default 0.5)\n"
            "  out.png              defaults to fg.png or fg_alpha.png\n";
}

int main(int argc, char** argv)
{
    if (argc < 6) { usage(argv[0]); return 1; }

    string inPath = argv[1];
    int rx = std::stoi(argv[2]);
    int ry = std::stoi(argv[3]);
    int rw = std::stoi(argv[4]);
    int rh = std::stoi(argv[5]);

    double marginPct = 0.0;
    bool alphaOut = false;
    double scale = 0.5;
    string outPath;

    for (int i = 6; i < argc; ++i) {
        string a = argv[i];
        if (a == "--margin" && i + 1 < argc) {
            marginPct = std::stod(argv[++i]) / 100.0;
            marginPct = std::max(0.0, std::min(marginPct, 1.0));
        }
        else if (a == "--alpha") {
            alphaOut = true;
        }
        else if (a == "--scale" && i + 1 < argc) {
            scale = std::stod(argv[++i]);
            scale = std::max(0.1, std::min(scale, 1.0));
        }
        else {
            outPath = a;
        }
    }

    if (outPath.empty())
        outPath = alphaOut ? "fg_alpha.png" : "fg.png";

    Mat src = imread(inPath);
    if (src.empty()) {
        cerr << "Cannot read " << inPath << endl;
        return 2;
    }

    if (marginPct > 0.0) {
        int dw = cvRound(rw * marginPct);
        int dh = cvRound(rh * marginPct);
        rx = std::max(0, rx - dw);
        ry = std::max(0, ry - dh);
        rw = std::min(src.cols - rx, rw + 2*dw);
        rh = std::min(src.rows - ry, rh + 2*dh);
    }

    Mat small;
    resize(src, small, Size(), scale, scale, INTER_AREA);

    Rect userRoi;
    userRoi.x      = cvRound(rx * scale);
    userRoi.y      = cvRound(ry * scale);
    userRoi.width  = cvRound(rw * scale);
    userRoi.height = cvRound(rh * scale);
    userRoi &= Rect(0, 0, small.cols, small.rows);

    Mat mask(small.size(), CV_8UC1, Scalar(GC_PR_BGD));
    Mat bgModel, fgModel;

    grabCut(small, mask, userRoi, bgModel, fgModel, 5, GC_INIT_WITH_RECT);

    Mat fgSmall = (mask == GC_FGD) | (mask == GC_PR_FGD);
    Mat fgMask;
    resize(fgSmall, fgMask, src.size(), 0, 0, INTER_NEAREST);

    Mat out;
    if (alphaOut) {
        cvtColor(src, out, COLOR_BGR2BGRA);
        for (int y = 0; y < out.rows; ++y) {
            Vec4b* pOut = out.ptr<Vec4b>(y);
            uchar*   m    = fgMask.ptr<uchar>(y);
            for (int x = 0; x < out.cols; ++x) {
                if (!m[x]) pOut[x][3] = 0;
            }
        }
    } else {
        out = Mat::zeros(src.size(), src.type());
        src.copyTo(out, fgMask);
    }

    imwrite(outPath, out);
    cout << "Saved " << outPath << endl;
    return 0;
}