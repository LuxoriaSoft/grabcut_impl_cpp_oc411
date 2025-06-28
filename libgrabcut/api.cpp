#include "api.h"
#include <opencv2/opencv.hpp>
#include <opencv2/photo.hpp>
#include <algorithm>
#include <string>

using namespace cv;

GRABCUT_API int grabcut_exec(
    const char* imagePath,
    const char* outPath,
    const int   rx,
    const int   ry,
    const int   rw,
    const int   rh,
    const int   margin
) {
    Mat src = imread(imagePath, IMREAD_COLOR);
    if (src.empty()) {
        return -1;
    }

    double marginPct = std::clamp(margin / 100.0, 0.0, 1.0);
    int x0 = rx, y0 = ry, w0 = rw, h0 = rh;
    if (marginPct > 0.0) {
        int dw = cvRound(rw * marginPct);
        int dh = cvRound(rh * marginPct);
        x0 = std::max(0, rx - dw);
        y0 = std::max(0, ry - dh);
        w0 = std::min(src.cols - x0, rw + 2*dw);
        h0 = std::min(src.rows - y0, rh + 2*dh);
    }

    double scale = 0.5;
    Mat small;
    resize(src, small, Size(), scale, scale, INTER_AREA);

    Rect userRoi;
    userRoi.x      = cvRound(x0 * scale);
    userRoi.y      = cvRound(y0 * scale);
    userRoi.width  = cvRound(w0 * scale);
    userRoi.height = cvRound(h0 * scale);
    userRoi &= Rect(0, 0, small.cols, small.rows);

    Mat mask(small.size(), CV_8UC1, Scalar(GC_PR_BGD));
    Mat bgModel, fgModel;
    grabCut(small, mask, userRoi, bgModel, fgModel, 5, GC_INIT_WITH_RECT);

    Mat fgSmall = (mask == GC_FGD) | (mask == GC_PR_FGD);
    Mat fgMask;
    resize(fgSmall, fgMask, src.size(), 0, 0, INTER_NEAREST);

    Mat out = Mat::zeros(src.size(), src.type());
    src.copyTo(out, fgMask);

    if (!imwrite(outPath, out)) {
        return -1;
    }

    return 0;
}
