//
// Created by Zhanibek Bakin on 26.06.2025.
//
#include <iostream>
#include "RateLimiter.h"

std::unique_ptr<RateLimiterManager> RateLimiterManager::instance_;
std::once_flag RateLimiterManager::initFlag_;
int main() {
    auto swrl = std::make_shared<FixedWindowRateLimiter>(10, 0);
    auto limiter = RateLimiterFactory::createRateLimiter("sliding", 5, 1000);
    if (limiter->allowRequest("client1")) {
        std::cout << "Request allowed\n";
    }

    if (RateLimiterManager::getInstance().allowRequest("client42")) {
        std::cout << "Allowed\n";
    } else {
        std::cout << "Blocked\n";
    }
}