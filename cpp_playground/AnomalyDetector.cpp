//
//  AnomalyDetector.cpp
//  cpp_playground
//
//  Created by Vihan Patil on 12/31/25.
//

#include "AnomalyDetector.hpp"
#include <iostream>

AnomalyDetector::AnomalyDetector(int size, double thresholdIn) :
    threshold(thresholdIn),
    runningSum(0.0),
    sqrSum(0.0),
    maxSize(size),
    head_index(0),
    count(0),
    window(size, 0.0)
    {}

double AnomalyDetector::getSTD(){
    return std::sqrt((sqrSum/count)-((runningSum/count)*(runningSum/count)));
}

bool AnomalyDetector::ingest(double amount){
    bool isAnomaly = false;
    if (count != 0 && count > maxSize*0.8) {
        if ((amount - (runningSum/count)) > threshold * getSTD()) {
            std::cout << "[ALERT] Transaction: " << amount << ", Abs Val - Mean: " << abs(amount - (runningSum/count)) << " > Limit: " << (threshold * getSTD()) << "\n";
            isAnomaly = true;
        }
    }
    if (count == maxSize) {
        runningSum -= window[head_index];
        sqrSum -= window[head_index]*window[head_index];
        count--;
    }
    window[head_index] = amount;
    runningSum += amount;
    sqrSum += amount*amount;
    head_index = (head_index + 1) % maxSize;
    count++;
    return isAnomaly;
}

double AnomalyDetector::getAvg(){return runningSum/count;}
