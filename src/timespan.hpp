namespace Config {

template<typename T>
class TimeSpan {
public:
    constexpr static TimeSpan from_seconds(T seconds) { return TimeSpan(seconds * 1000); }
    constexpr T milliseconds() { return _milliseconds; }
    constexpr T seconds() { return _milliseconds / 1000; }
private:
    constexpr explicit TimeSpan(T milliseconds): _milliseconds(milliseconds) {}

    T _milliseconds;
};

}