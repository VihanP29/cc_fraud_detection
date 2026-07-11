//
//  main.cpp
//  cpp_playground
//
//  Created by Vihan Patil on 12/30/25.
//

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include "AnomalyDetector.hpp"

struct CliOptions {
    bool streamMode = false;
    int streamCount = 10000;
};

void printUsage(const std::string &programName) {
    std::cout << "Usage: " << programName << " [--stream <num_transactions>]\n"
              << "  --stream <num_transactions>   Output a CSV stream for dashboard consumption\n"
              << "  --help                       Show this usage information\n";
}

CliOptions parseArguments(int argc, const char * argv[]) {
    CliOptions options;
    std::string programName = argc > 0 ? argv[0] : "engine";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stream") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing transaction count after --stream\n");
            }
            options.streamMode = true;
            try {
                options.streamCount = std::stoi(argv[++i]);
            } catch (const std::exception &) {
                throw std::runtime_error("Invalid number supplied to --stream\n");
            }
            if (options.streamCount <= 0) {
                throw std::runtime_error("Stream transaction count must be positive\n");
            }
        } else if (arg == "--help" || arg == "-h") {
            printUsage(programName);
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + arg + "\n");
        }
    }
    return options;
}

double randomAmount(double minValue, double maxValue, std::mt19937 &rng) {
    std::uniform_real_distribution<double> dist(minValue, maxValue);
    return dist(rng);
}

double benchmarkTPS(int numTransactions, int windowLen, double sensitivity, std::mt19937 &rng) {
    AnomalyDetector benchmarkEngine(windowLen, sensitivity);
    benchmarkEngine.setVerbose(false);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numTransactions; ++i) {
        benchmarkEngine.ingest(randomAmount(40.0, 60.0, rng));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    if (elapsedUs == 0) {
        elapsedUs = 1;
    }
    return (static_cast<double>(numTransactions) * 1'000'000.0) / static_cast<double>(elapsedUs);
}

void streamTransactions(int numTransactions, int windowLen, double sensitivity, std::mt19937 &rng) {
    AnomalyDetector streamingEngine(windowLen, sensitivity);
    streamingEngine.setVerbose(false);
    std::cout << "Transaction_ID,Amount,Is_Anomaly,Current_Mean,Current_STD\n";
    for (int i = 1; i <= numTransactions; ++i) {
        double amount = randomAmount(40.0, 60.0, rng);
        if (i % 5000 == 0) {
            amount = 5000.0;
        }
        bool isAnomaly = streamingEngine.ingest(amount);
        std::cout << i << "," << std::fixed << std::setprecision(2)
                  << amount << "," << (isAnomaly ? 1 : 0) << ","
                  << streamingEngine.getAvg() << "," << streamingEngine.getSTD() << "\n";
    }
}

int main(int argc, const char * argv[]) {
    constexpr int windowLen = 50;
    constexpr double sensitivity = 3.0;
    std::random_device rd;
    std::mt19937 rng(rd());

    CliOptions options;
    try {
        options = parseArguments(argc, argv);
    } catch (const std::exception &ex) {
        std::cerr << ex.what();
        return 1;
    }

    if (options.streamMode) {
        double tps = benchmarkTPS(1'000'000, windowLen, sensitivity, rng);
        std::cout << "TPS," << std::fixed << std::setprecision(2) << tps << "\n";
        streamTransactions(options.streamCount, windowLen, sensitivity, rng);
        return 0;
    }

    std::cout << "--- SYSTEM STARTUP (Window: " << windowLen << ") ---\n";
    AnomalyDetector engine(windowLen, sensitivity);

    std::cout << "\n[PHASE 1] Ingesting Normal Traffic ($40-$60)...\n";
    for (int i = 0; i < 10000; i++) {
        double normalAmount = randomAmount(40.0, 60.0, rng);
        bool alerted = engine.ingest(normalAmount);
        if (i % 100 == 0) {
            std::cout << "  > Step " << i << ": Added $" << std::fixed << std::setprecision(2) << normalAmount
                      << " | Current Mean: " << engine.getAvg()
                      << " | Current StdDev: " << engine.getSTD() << "\n";
        }
        (void)alerted;
    }

    std::cout << "\n[PHASE 2] Injecting Anomaly ($5000.00)...\n";
    double hugeAmount = 5000.00;
    bool fraudDetected = engine.ingest(hugeAmount);
    if (fraudDetected) {
        std::cout << "✅ SUCCESS: Fraud Detected on $" << hugeAmount << "\n";
    } else {
        std::cout << "❌ FAILURE: Fraud ($" << hugeAmount << ") slipped through!\n";
    }

    std::cout << "\n[PHASE 3] Injecting Low Value ($0.50)...\n";
    double tinyAmount = 0.50;
    bool lowDetected = engine.ingest(tinyAmount);
    if (lowDetected) {
        std::cout << "⚠️ WARNING: Low value ($" << tinyAmount << ") was flagged! (Check your absolute value logic)\n";
    } else {
        std::cout << "✅ SUCCESS: Low value ($" << tinyAmount << ") was ignored correctly.\n";
    }

    double tps = benchmarkTPS(1'000'000, windowLen, sensitivity, rng);
    std::cout << "\n[PHASE 4] Benchmark: Transactions Per Second (TPS) = " << std::fixed << std::setprecision(2)
              << tps << "\n";
    return 0;
}

