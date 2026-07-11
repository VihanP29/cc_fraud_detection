//
//  AnomalyDetector.cpp
//  cpp_playground
//
//  Created by Vihan Patil on 12/31/25.
//

#include "AnomalyDetector.hpp"
#include <cmath>

AnomalyDetector::AnomalyDetector(int size, double thresholdIn) :
    threshold(thresholdIn),
    runningSum(0.0),
    sqrSum(0.0),
    maxSize(size),
    head_index(0),
    count(0),
    verbose(true),
    window(size, 0.0)
    {}

void AnomalyDetector::setVerbose(bool enable) {
    verbose = enable;
}

double AnomalyDetector::getSTD(){
    if (count <= 1) {
        return 0.0;
    }
    double mean = runningSum / count;
    double variance = (sqrSum / count) - (mean * mean);
    return variance > 0.0 ? std::sqrt(variance) : 0.0;
}

bool AnomalyDetector::ingest(double amount){
    bool isAnomaly = false;
    if (count != 0 && count > maxSize * 0.8) {
        double mean = runningSum / count;
        double deviation = amount - mean;
        double limit = threshold * getSTD();
        if (deviation > limit && limit > 0.0) {
            if (verbose) {
                std::cout << "[ALERT] Transaction: " << amount
                          << ", Abs Val - Mean: " << std::abs(deviation)
                          << " > Limit: " << limit << "\n";
            }
            isAnomaly = true;
        }
    }
    if (count == maxSize) {
        runningSum -= window[head_index];
        sqrSum -= window[head_index] * window[head_index];
        count--;
    }
    window[head_index] = amount;
    runningSum += amount;
    sqrSum += amount * amount;
    head_index = (head_index + 1) % maxSize;
    count++;
    return isAnomaly;
}

double AnomalyDetector::getAvg(){
    return count > 0 ? runningSum / count : 0.0;
}
