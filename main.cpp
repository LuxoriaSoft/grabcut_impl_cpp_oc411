#include <opencv2/opencv.hpp>
#include <opencv2/photo.hpp>
#include <iostream>
#include <algorithm>

using namespace cv;
using namespace std;

// tweak these defaults to balance speed vs. quality
constexpr int DEFAULT_ITERS   = 5;    // default graph-cut passes
constexpr int PAD_PIXELS      = 10;   // padding around user ROI

struct AppState {
    Mat        src, scaled, display;
    Rect       userROI;
    bool       roiReady = false;
    double     scale;
    bool       alphaOut;
    string     outPath;
};

static void printUsage(const char* prog) {
    cerr << "Usage:\n  " << prog
         << " <in.jpg> [--alpha] [--scale s] [--iters n] [out.png]\n"
            "    --alpha    output RGBA PNG\n"
            "    --scale s  downscale factor (0.1–1.0, default 0.5)\n"
            "    --iters n  GrabCut iterations (default " << DEFAULT_ITERS << ")\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    AppState S;
    S.scale    = 0.5;
    S.alphaOut = false;
    int iters   = DEFAULT_ITERS;

    S.src = imread(argv[1]);
    if (S.src.empty()) {
        cerr << "Error: could not load '" << argv[1] << "'\n";
        return 2;
    }

    for (int i = 2; i < argc; ++i) {
        string a = argv[i];
        if      (a == "--alpha") {
            S.alphaOut = true;
        }
        else if (a == "--scale" && i+1<argc) {
            S.scale = clamp(stod(argv[++i]), 0.1, 1.0);
        }
        else if (a == "--iters" && i+1<argc) {
            iters = max(1, stoi(argv[++i]));
        }
        else {
            S.outPath = a;
        }
    }
    if (S.outPath.empty()) {
        S.outPath = S.alphaOut ? "fg_alpha.png" : "fg.png";
    }

    setUseOptimized(true);
    setNumThreads(getNumberOfCPUs());

    resize(S.src, S.scaled, Size(), S.scale, S.scale, INTER_AREA);
    S.display = S.scaled.clone();

    namedWindow("GrabCut", WINDOW_AUTOSIZE);

    auto onMouse = [](int event, int x, int y, int, void* userdata) {
        AppState& st = *reinterpret_cast<AppState*>(userdata);
        static Point p0;

        if (event == EVENT_LBUTTONDOWN) {
            p0 = Point(x, y);
            st.roiReady = false;
        }
        else if (event == EVENT_MOUSEMOVE && !st.roiReady) {
            Mat tmp = st.scaled.clone();
            rectangle(tmp, p0, Point(x, y), Scalar(0,255,0), 2);
            imshow("GrabCut", tmp);
        }
        else if (event == EVENT_LBUTTONUP) {
            Rect raw(p0, Point(x, y));
            Rect bounds(0, 0, st.scaled.cols, st.scaled.rows);
            st.userROI = raw & bounds;
            st.roiReady = true;

            st.display = st.scaled.clone();
            rectangle(st.display, st.userROI, Scalar(0,255,0), 2);
            imshow("GrabCut", st.display);
        }
    };

    setMouseCallback("GrabCut", onMouse, &S);
    imshow("GrabCut", S.display);

    cout << "Draw ROI, then press ENTER/SPACE. ESC to quit.\n";
    while (true) {
        int k = waitKey(1);
        if ((k == 13 || k == 32) && S.roiReady) break;
        if (k == 27) {
            cout << "Aborted.\n";
            return 0;
        }
    }
    destroyWindow("GrabCut");

    if (S.userROI.width < 2 || S.userROI.height < 2) {
        cerr << "Error: ROI too small.\n";
        return 3;
    }

    Rect R = S.userROI;
    R.x      = max(0, R.x - PAD_PIXELS);
    R.y      = max(0, R.y - PAD_PIXELS);
    R.width  = min(S.scaled.cols - R.x, R.width + 2*PAD_PIXELS);
    R.height = min(S.scaled.rows - R.y, R.height + 2*PAD_PIXELS);

    Mat patch    = S.scaled(R).clone();
    // initialize all as probable background
    Mat maskPatch(patch.size(), CV_8UC1, Scalar(GC_PR_BGD));
    // mark inner box as probable foreground
    Rect inner(PAD_PIXELS, PAD_PIXELS, S.userROI.width, S.userROI.height);
    maskPatch(inner).setTo(Scalar(GC_PR_FGD));
    maskPatch.row(0).setTo(Scalar(GC_BGD));
    maskPatch.row(patch.rows-1).setTo(Scalar(GC_BGD));
    maskPatch.col(0).setTo(Scalar(GC_BGD));
    maskPatch.col(patch.cols-1).setTo(Scalar(GC_BGD));

    // --- run GrabCut on the small patch ---
    cout << "Running GrabCut (" << iters << " iterations) on ROI... " << flush;
    Mat bgModel, fgModel;
    try {
        grabCut(patch, maskPatch, Rect(), bgModel, fgModel, iters, GC_INIT_WITH_MASK);
    }
    catch (const cv::Exception& e) {
        cerr << "\nGrabCut failed: " << e.what() << endl;
        return 4;
    }
    cout << "done.\n";

    Mat fgSmall = (maskPatch == GC_FGD) | (maskPatch == GC_PR_FGD);
    Mat fullMask = Mat::zeros(S.scaled.size(), CV_8UC1);
    fgSmall.copyTo(fullMask(R));

    Mat finalMask;
    resize(fullMask, finalMask, S.src.size(), 0, 0, INTER_NEAREST);

    Mat outImg;
    if (S.alphaOut) {
        cvtColor(S.src, outImg, COLOR_BGR2BGRA);
        for (int y = 0; y < outImg.rows; ++y) {
            Vec4b* pRow = outImg.ptr<Vec4b>(y);
            uchar*   mRow = finalMask.ptr<uchar>(y);
            for (int x = 0; x < outImg.cols; ++x)
                if (!mRow[x]) pRow[x][3] = 0;
        }
    } else {
        outImg = Mat::zeros(S.src.size(), S.src.type());
        S.src.copyTo(outImg, finalMask);
    }

    if (!imwrite(S.outPath, outImg)) {
        cerr << "Error: failed to save '" << S.outPath << "'\n";
        return 5;
    }

    cout << "Saved result to " << S.outPath << "\n";
    return 0;
}
