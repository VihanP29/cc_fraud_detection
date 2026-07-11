cmake_minimum_required(VERSION 3.10)

project(FraudEngine)

set(CMAKE_CXX_STANDARD 17)

add_executable(engine
    main.cpp
    AnomalyDetector.cpp
)
