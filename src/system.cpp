#include "system.h"

#include <algorithm>  // std::sort
#include <string>     // std::string
#include <vector>     // std::vector

#include "linux_parser.h"
#include "process.h"
#include "processor.h"

System::System() {
  cpu_ = Cpu();
  processes_ = std::vector<Process>();
}

Processor& System::Cpu() { return cpu_; }

std::vector<Process>& System::Processes() {
  processes_.clear();
  auto processes = std::vector<Process>();
  for (auto&& pid : LinuxParser::Pids()) {
    processes.emplace_back(Process(pid));
  }
  std::sort(processes.begin(), processes.end());
  for (auto i = 0; i < 10; ++i) {
    processes_.emplace_back(processes.back());
    processes.pop_back();
  }
  return processes_;
}

std::string System::Kernel() { return LinuxParser::Kernel(); }

float System::MemoryUtilization() { return LinuxParser::MemoryUtilization(); }

std::string System::OperatingSystem() { return LinuxParser::OperatingSystem(); }

int System::RunningProcesses() { return LinuxParser::RunningProcesses(); }

int System::TotalProcesses() { return LinuxParser::TotalProcesses(); }

long System::UpTime() { return LinuxParser::UpTime(); }
