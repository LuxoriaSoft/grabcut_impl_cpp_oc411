#ifndef FOREGROUNDEXTRACTOR_HPP
#define FOREGROUNDEXTRACTOR_HPP

#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <vector>

namespace fg {

/**
 * Automatic GrabCut fore-ground extractor.
 *
 * 1.  Samples a <borderPx>-wide frame; assumes it is background.
 * 2.  Any pixel within <thresh> of the frame’s mean colour is “probable BG”.
 * 3.  Runs GrabCut with that mask.
 * 4.  Returns a BGRA image (alpha = 0 for BG).
 */
class ForegroundExtractor {
public:
    ForegroundExtractor(int iterations = 5,
                        int borderPx   = 8,
                        double thresh  = 25.0)
        : iters_(iterations), border_(borderPx), thr_(thresh) {}

    void   setIterations(int v) { iters_  = v; }
    void   setBorder    (int v) { border_ = v; }
    void   setThreshold (double v) { thr_ = v; }

    /** Extract and return BGRA foreground image */
    cv::Mat extract(const cv::Mat& src) const {
        validate(src);

        // -------- 1. mean colour of the frame --------------------------------
        cv::Mat srcBGR;
        if (src.channels() == 4)
            cv::cvtColor(src, srcBGR, cv::COLOR_BGRA2BGR);
        else
            srcBGR = src;

        // build a mask for the border frame
        cv::Mat frameMask(src.size(), CV_8UC1, cv::Scalar(0));
        frameMask.rowRange(0, border_).setTo(255);
        frameMask.rowRange(src.rows - border_, src.rows).setTo(255);
        frameMask.colRange(0, border_).setTo(255);
        frameMask.colRange(src.cols - border_, src.cols).setTo(255);

        cv::Scalar meanBGR = cv::mean(srcBGR, frameMask);

        // -------- 2. colour distance to frame mean ---------------------------
        cv::Mat srcF;
        srcBGR.convertTo(srcF, CV_32F);
        cv::Mat meanMat(srcF.size(), CV_32FC3, meanBGR);
        cv::Mat diff, dist2;
        cv::subtract(srcF, meanMat, diff);
        cv::multiply(diff, diff, diff);
        std::vector<cv::Mat> ch; cv::split(diff, ch);
        dist2 = ch[0] + ch[1] + ch[2];
        cv::Mat dist; cv::sqrt(dist2, dist);

        cv::Mat probBG;
        cv::threshold(dist, probBG, thr_, 255, cv::THRESH_BINARY_INV);
        probBG.convertTo(probBG, CV_8UC1);

        // -------- 3. build GrabCut mask --------------------------------------
        cv::Mat gcMask(src.size(), CV_8UC1, cv::Scalar(cv::GC_PR_FGD));   // default probable FG

        // definite BG (frame)
        gcMask.setTo(cv::Scalar(cv::GC_BGD), frameMask);

        // probable BG (colour-similar pixels)
        gcMask.setTo(cv::Scalar(cv::GC_PR_BGD), probBG);

        // -------- 4. GrabCut --------------------------------------------------
        cv::Mat bgdModel, fgdModel;
        cv::grabCut(srcBGR, gcMask, cv::Rect(), bgdModel, fgdModel,
                    iters_, cv::GC_INIT_WITH_MASK);

        cv::Mat binMask = (gcMask == cv::GC_FGD) | (gcMask == cv::GC_PR_FGD);
        binMask.convertTo(binMask, CV_8UC1, 255);

        // -------- 5. pack BGRA output ----------------------------------------
        std::vector<cv::Mat> bgr; cv::split(srcBGR, bgr);
        bgr.push_back(binMask);                     // alpha
        cv::Mat out;
        cv::merge(bgr, out);
        return out;
    }

    /** Convenience wrapper: save PNG directly. */
    bool extractAndSave(const cv::Mat& src, const std::string& path) const {
        return cv::imwrite(path, extract(src));
    }

private:
    int    iters_;
    int    border_;
    double thr_;

    void validate(const cv::Mat& src) const {
        if (src.empty())
            throw std::invalid_argument("input image is empty");
        if (src.channels() != 3 && src.channels() != 4)
            throw std::invalid_argument("image must have 3 or 4 channels");
        if (src.rows < 2*border_+1 || src.cols < 2*border_+1)
            throw std::invalid_argument("image too small for given border");
    }
};

} // namespace fg
#endif // FOREGROUNDEXTRACTOR_HPP
