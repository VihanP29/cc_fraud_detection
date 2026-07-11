//
//  AnomalyDetector.hpp
//  cpp_playground
//
//  Created by Vihan Patil on 12/31/25.
//

#pragma once
#include <cmath>
#include <iostream>
#include <vector>

class AnomalyDetector{
private:
    double runningSum;
    double sqrSum;
    double threshold;
    std::vector<double> window;
    long maxSize;
    long head_index;
    long count;
    bool verbose;
public:
    AnomalyDetector(int size, double thresholdIn);
    void setVerbose(bool enable);
    bool ingest(double amount);
    double getAvg();
    double getSTD();
};
