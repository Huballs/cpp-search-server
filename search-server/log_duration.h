#pragma once
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <map>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x) 

/*Такого трюка для определения UNIQUE_VAR_NAME_PROFILE — макроса, генерирующего уникальное 
имя переменной — требуют довольно запутанные правила раскрытия в C++. Параметры макроса при 
склеивании заменяются на то, что в них было подставлено без изменения. Те параметры, которые 
не склеиваются, раскрываются, то есть полностью подставляются до того момента, пока в них не 
останется макросов. Чтобы достичь желаемого, нужно, чтобы __LINE__ побывал параметром два 
раза: в первый раз он раскроется в номер строки, во второй раз номер строки приклеится к 
имени переменной.*/


class LogDuration {
public:
    using Clock = std::chrono::steady_clock;

    LogDuration(std::string_view id = "", std::ostream& dst_stream = std::cerr);

    ~LogDuration();

private:
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& dst_stream_;
}; 

class LogIncremental {
public:
    using Clock = std::chrono::steady_clock;

    LogIncremental(){

    };

    void increment(const std::string& id, const std::chrono::duration<int64_t, std::nano> time){
        id_times[id] += time;
        id_counts[id]++;
    }

    int64_t getIncrement(const std::string& id){
        auto it = id_times.find(id);
        if (it != id_times.end())
            return std::chrono::duration_cast<std::chrono::milliseconds>(id_times[id]).count();
        return 0;
    }

    int64_t getCounts(const std::string& id){
        auto it = id_counts.find(id);
        if (it != id_counts.end())
            return id_counts[id];

        return 0;
    }

    private:
    std::map<std::string, std::chrono::duration<int64_t, std::nano> > id_times;
    std::map<std::string, int64_t> id_counts;
};

class Increment {
public:
    using Clock = std::chrono::steady_clock;
    
    explicit Increment(LogIncremental& log, const std::string id) : log_(log), id_(id) {
    };

    ~Increment(){
        log_.increment(id_, Clock::now() - start_time_);
    };

    private:
    const Clock::time_point start_time_ = Clock::now();
    LogIncremental& log_;
    std::string id_;
};