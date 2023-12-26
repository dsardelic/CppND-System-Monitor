#include "system.h"

#include <string>  // std::string
#include <vector>  // std::vector

#include "linux_parser.h"
#include "process.h"
#include "processor.h"

Processor& System::Cpu() { return cpu_; }

std::vector<Process>& System::Processes() { return processes_; }

std::string System::Kernel() { return LinuxParser::Kernel(); }

float System::MemoryUtilization() { return LinuxParser::MemoryUtilization(); }

std::string System::OperatingSystem() { return LinuxParser::OperatingSystem(); }

int System::RunningProcesses() { return LinuxParser::RunningProcesses(); }

int System::TotalProcesses() { return LinuxParser::TotalProcesses(); }

long System::UpTime() { return LinuxParser::UpTime(); }
