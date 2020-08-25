#ifndef Date_hh
#define Date_hh

#include <cstring>
#include <ctime>
#include <string>

class Date {
  
 public:
  Date(time_t time) { set(time); }
  Date(const Date& date) { set(date.m_time); }
  Date() { set(); }
  ~Date() {}
  
 public:
  void set() {
    m_time = time(NULL);
    m_tm = localtime(&m_time);
  }
  void set(time_t time) {
    m_time = time;
    m_tm = localtime(&m_time);
  }
  time_t get() const { return m_time; }
  int getSecond() const { return m_tm->tm_sec; }
  int getMinitue() const { return m_tm->tm_min; }
  int getHour() const { return m_tm->tm_hour; }
  int getDay() const { return m_tm->tm_mday; }
  int getMonth() const { return m_tm->tm_mon+1; }
  int getYear() const { return m_tm->tm_year+1900; }
  const char* toString(const char* format = NULL) const {
    memset(m_str, 0, sizeof(m_str));
    if (format == NULL) {
      strftime(m_str, 31, "%Y-%m-%d %H:%M:%S", m_tm);
    } else {
      strftime(m_str, 31, format, m_tm);
    }
    return m_str;
  }
 public:
  const Date& operator=(const Date& date)
    {
      set(date.m_time);
      return *this;
    }
  
 private:
  time_t m_time;
  struct tm* m_tm;
  mutable char m_str[31];
  
};

#endif
