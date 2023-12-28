#include "process.h"

#include <unistd.h>  // sysconf(_SC_CLK_TCK)

#include <string>

#include "linux_parser.h"

int Process::Pid() const { return pid_; }

float Process::CpuUtilization() const {
  float activeTime = static_cast<float>(LinuxParser::ActiveJiffies(pid_)) /
                     sysconf(_SC_CLK_TCK);
  return activeTime / LinuxParser::UpTime(pid_);
}

std::string Process::Command() { return LinuxParser::Command(pid_); }

std::string Process::Ram() { return LinuxParser::Ram(pid_); }

std::string Process::User() { return LinuxParser::User(pid_); }

long int Process::UpTime() { return LinuxParser::UpTime(pid_); }

bool Process::operator<(Process const& rhs) const {
  return this->CpuUtilization() < rhs.CpuUtilization();
}
