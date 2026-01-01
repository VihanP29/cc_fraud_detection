//
//  main.cpp
//  cpp_playground
//
//  Created by Vihan Patil on 12/30/25.
//

#include <iostream>
#include <iomanip> // For std::setprecision
#include "AnomalyDetector.hpp"

int main(int argc, const char * argv[]) {
    // 1. Setup: Window size 20, Threshold 3.0 (3 Standard Deviations)
    int windowLen = 50;
    double sensitivity = 3.0;
    
    std::cout << "--- SYSTEM STARTUP (Window: " << windowLen << ") ---\n";
    AnomalyDetector engine(windowLen, sensitivity);
    
    // 2. Training Phase: Feed "Normal" data ($40 - $60 range)
    // We ignore alerts here while the model "warms up"
    std::cout << "\n[PHASE 1] Ingesting Normal Traffic ($40-$60)...\n";
    for(int i = 0; i < 10000; i++){
        // Generate random amount between 40 and 60
        double normalAmount = 20.0 + (rand() % 100);
        bool alerted = engine.ingest(normalAmount);
        
        // Print status every 10 transactions
        if (i % 100 == 0) {
            std::cout << "  > Step " << i << ": Added $" << normalAmount
                      << " | Current Mean: " << engine.getAvg()
                      << " | Current StdDev: " << engine.getSTD() << "\n";
        }
    }

    // 3. Attack Phase: The Fraudulent Transaction
    std::cout << "\n[PHASE 2] Injecting Anomaly ($5000.00)...\n";
    double hugeAmount = 5000.00;
    bool fraudDetected = engine.ingest(hugeAmount);
    
    if (fraudDetected) {
        std::cout << "✅ SUCCESS: Fraud Detected on $" << hugeAmount << "\n";
    } else {
        std::cout << "❌ FAILURE: Fraud ($" << hugeAmount << ") slipped through!\n";
    }

    // 4. Low Value Phase: The "Coffee" Transaction
    // This tests if your One-Tailed test works (should NOT alert on low values)
    std::cout << "\n[PHASE 3] Injecting Low Value ($0.50)...\n";
    double tinyAmount = 0.50;
    bool lowDetected = engine.ingest(tinyAmount);
    
    if (lowDetected) {
        std::cout << "⚠️ WARNING: Low value ($" << tinyAmount << ") was flagged! (Check your absolute value logic)\n";
    } else {
        std::cout << "✅ SUCCESS: Low value ($" << tinyAmount << ") was ignored correctly.\n";
    }

    return 0;
}

