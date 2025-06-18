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
         << " <in.jpg> [--alpha] [--scale s] [out.png]\n\n"
            "  --alpha     output RGBA PNG with transparent background\n"
            "  --scale s   downscale factor (0.1–1.0, default 0.5)\n"
            "  out.png     defaults to fg.png or fg_alpha.png\n";
}

int main(int argc, char** argv)
{
    if (argc < 2) { usage(argv[0]); return 1; }

    // parse args
    string inPath = argv[1], outPath;
    bool alphaOut = false;
    double scale = 0.5;
    for (int i = 2; i < argc; ++i) {
        string a = argv[i];
        if (a == "--alpha") {
            alphaOut = true;
        }
        else if (a == "--scale" && i+1 < argc) {
            scale = std::stod(argv[++i]);
            scale = std::max(0.1, std::min(scale, 1.0));
        }
        else {
            outPath = a;
        }
    }
    if (outPath.empty())
        outPath = alphaOut ? "fg_alpha.png" : "fg.png";

    // load full-res
    Mat src = imread(inPath);
    if (src.empty()) {
        cerr << "Cannot read " << inPath << endl;
        return 2;
    }

    Mat small;
    resize(src, small, Size(), scale, scale, INTER_AREA);

    Mat smallMask(small.size(), CV_8UC1, Scalar(GC_PR_BGD));

    int margin = std::max(1, std::min(small.rows, small.cols) / 20);
    smallMask(Range(0, margin),                    Range::all())          = GC_PR_BGD;
    smallMask(Range(small.rows - margin, small.rows), Range::all())      = GC_PR_BGD;
    smallMask(Range::all(),                       Range(0, margin))       = GC_PR_BGD;
    smallMask(Range::all(),                       Range(small.cols - margin, small.cols)) = GC_PR_BGD;

    int cw = small.cols / 2, ch = small.rows / 2;
    int cx = (small.cols - cw) / 2, cy = (small.rows - ch) / 2;
    Rect centerSeed(cx, cy, cw, ch);
    smallMask(centerSeed).setTo(GC_PR_FGD);

    // run GrabCut on the small image
    Mat bgModel, fgModel;
    grabCut(small, smallMask, Rect(), bgModel, fgModel, 5, GC_INIT_WITH_MASK);

    // build and upsample the foreground mask
    Mat smallFg = (smallMask == GC_FGD) | (smallMask == GC_PR_FGD);
    Mat fgMask;
    resize(smallFg, fgMask, src.size(), 0, 0, INTER_NEAREST);

    // compose output
    Mat out;
    if (alphaOut) {
        cvtColor(src, out, COLOR_BGR2BGRA);
        for (int y = 0; y < out.rows; ++y) {
            Vec4b* pOut = out.ptr<Vec4b>(y);
            uchar*   m   = fgMask.ptr<uchar>(y);
            for (int x = 0; x < out.cols; ++x) {
                if (!m[x])  // background pixel
                    pOut[x][3] = 0;  // transparent
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
