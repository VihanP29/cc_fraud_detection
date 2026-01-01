//
//  AnomalyDetector.hpp
//  cpp_playground
//
//  Created by Vihan Patil on 12/31/25.
//

#pragma once
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
public:
    AnomalyDetector(int size, double thresholdIn);
    bool ingest(double amount);
    double getAvg();
    double getSTD();
};
