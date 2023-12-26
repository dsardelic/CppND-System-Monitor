#include "format.h"

#include <iomanip>  // std::setfill, std::setw
#include <sstream>  // std::stringstream
#include <string>   // std::string

std::string Format::ElapsedTime(long seconds) {
  long hours = seconds / 3600;
  hours %= 100;  // display limited to 2 digits
  seconds %= 3600;
  short minutes = seconds / 60;
  seconds %= 60;

  std::stringstream ss;
  ss << std::setfill('0');
  ss << std::setw(2) << hours << ":";
  ss << std::setw(2) << minutes << ":";
  ss << std::setw(2) << seconds;
  return ss.str();
}