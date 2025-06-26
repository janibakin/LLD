//
// Created by Zhanibek Bakin on 26.06.2025.
//

#ifndef RATELIMITER_H
#define RATELIMITER_H
#include <string>
#include <unordered_map>
#include <queue>

using Clock = std::chrono::steady_clock;
using Time = std::chrono::steady_clock::time_point;

class RateLimiter {
public:
    RateLimiter() = default;
    virtual bool allowRequest(const std::string& clientId) = 0;
    virtual ~RateLimiter() = default;
};

class FixedWindowRateLimiter final : public RateLimiter {
public:
    FixedWindowRateLimiter(const int maxRequests, const long windowSizeInMills) :
    maxRequests_(maxRequests), windowSize_(std::chrono::milliseconds(windowSizeInMills)) {}

    bool allowRequest(const std::string& clientId) override {
        auto now = Clock::now();
        auto& windowStart = windowStartTimes_[clientId];
        auto& requestCount = requestCounts_[clientId];
        if (windowStart == Clock::time_point{}) {
            windowStart = now;
            requestCount = 0;
        }
        // expired window
        if (now - windowStart >= windowSize_) {
            windowStart = now;
            requestCount = 0;
        }

        if (requestCount < maxRequests_) {
            ++requestCount;
            return true;
        }
        return false;
    }
private:
    int maxRequests_;
    std::chrono::milliseconds windowSize_;
    std::unordered_map<std::string, int> requestCounts_;
    std::unordered_map<std::string, Time> windowStartTimes_;

};


class SlidingWindowRateLimiter final : public RateLimiter {
public:
    SlidingWindowRateLimiter(const int maxRequests, const long windowSizeInMills) :
    maxRequests_(maxRequests), windowSize_(std::chrono::milliseconds(windowSizeInMills)) {}

    bool allowRequest(const std::string& clientId) override {
        auto now = Clock::now();
        auto& timestamps = requestTimestamps[clientId];
        // timestamps is where the oldest timestamp is
        while (!timestamps.empty() && now - timestamps.front() > windowSize_) {
            timestamps.pop();
        }
        if (timestamps.size() < maxRequests_) {
            timestamps.push(now);
            return true;
        }
        return false;
    }
private:
    int maxRequests_;
    std::chrono::milliseconds windowSize_;
    std::unordered_map<std::string, std::queue<Time>> requestTimestamps{};
};

class RateLimiterFactory {
public:
    static std::unique_ptr<RateLimiter> createRateLimiter(const std::string& type, int maxRequests, const long windowSizeInMills) {
        std::string lowerType = toLower(type);
        switch (hash(lowerType)) {
            case RateLimiterType::FIXED :
                return std::make_unique<FixedWindowRateLimiter>(maxRequests, windowSizeInMills);
            case RateLimiterType::SLIDING :
                return std::make_unique<SlidingWindowRateLimiter>(maxRequests, windowSizeInMills);
        }
        throw std::invalid_argument("Unknown rate limiter type: " + type);
    }
private:
    enum class RateLimiterType {
        FIXED,
        SLIDING
    };
    static RateLimiterType hash(const std::string& str ) {
        if (str == "fixed") return RateLimiterType::FIXED;
        if (str == "sliding") return RateLimiterType::SLIDING;
        throw std::invalid_argument("Unknown rate limiter type: " + str);
    }
    static std::string toLower(const std::string& type) {
        std::string lower {type};
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }
};

class RateLimiterManager {
public:
    static RateLimiterManager& getInstance() {
        std::call_once(initFlag_, []() {
            instance_.reset(new RateLimiterManager());
        });
        return *instance_;
    }

    [[nodiscard]] bool allowRequest(const std::string& clientId) const {
        return rateLimiter_->allowRequest(clientId);
    }
private:
    RateLimiterManager() {
        rateLimiter_ = RateLimiterFactory::createRateLimiter("Sliding", 100, 6000);
    }
    static std::unique_ptr<RateLimiterManager> instance_;
    static std::once_flag initFlag_;
    std::unique_ptr<RateLimiter> rateLimiter_;
};

#endif //RATELIMITER_H
