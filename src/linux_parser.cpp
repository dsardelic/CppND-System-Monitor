#include "linux_parser.h"

#include <dirent.h>  // DIR, opendir

#include <algorithm>  // std::unique
#include <fstream>    // std::ifstream
#include <sstream>    // std::istringstream, std::stringstream
#include <string>     // std::string, std::stol
#include <vector>     // std:vector

template <char Duplicate>
bool BothCharsAre(char lhs, char rhs) {
  return lhs == rhs && lhs == Duplicate;
};

std::string LinuxParser::OperatingSystem() {
  std::string line;
  std::string key;
  std::string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

std::string LinuxParser::Kernel() {
  std::string os, kernel, version;
  std::string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

std::vector<int> LinuxParser::Pids() {
  std::vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      std::string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = std::stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() {
  unsigned long memTotal{0UL}, memAvailable{0UL};
  std::ifstream meminfo_file{kProcDirectory + kMeminfoFilename};
  if (meminfo_file.is_open()) {
    bool memTotalFound{false}, memAvailableFound{false};
    std::string line;
    while (getline(meminfo_file, line) &&
           !(memTotalFound && memAvailableFound)) {
      line.erase(std::unique(line.begin(), line.end(), BothCharsAre<' '>),
                 line.end());  // eliminate duplicate spaces
      std::stringstream ss{line};
      std::string first_token;
      ss >> first_token;
      if (first_token == "MemTotal:") {
        ss >> memTotal;
        memTotalFound = true;
      } else if (first_token == "MemAvailable:") {
        ss >> memAvailable;
        memAvailableFound = true;
      }
    }
    meminfo_file.close();
  }
  return static_cast<float>(memTotal - memAvailable) / memTotal;
}

long LinuxParser::UpTime() {
  long upTime{0L};
  std::ifstream uptime_file{kProcDirectory + kUptimeFilename};
  if (uptime_file.is_open()) {
    std::string line;
    getline(uptime_file, line);
    upTime = std::stol(line.substr(0, line.find_first_of(' ')));
    uptime_file.close();
  }
  return upTime;
}

unsigned long LinuxParser::Jiffies() { return ActiveJiffies() + IdleJiffies(); }

// TODO: Read and return the number of active jiffies for a PID
// REMOVE: [[maybe_unused]] once you define the function
unsigned long LinuxParser::ActiveJiffies(int pid [[maybe_unused]]) { return 0; }

unsigned long LinuxParser::ActiveJiffies() {
  unsigned long result{0UL};
  auto cpuUtilization{CpuUtilization()};
  for (const auto& cpuState :
       {CPUStates::kUser_, CPUStates::kNice_, CPUStates::kSystem_,
        CPUStates::kIRQ_, CPUStates::kSoftIRQ_, CPUStates::kSteal_})
    result += std::stoul(cpuUtilization[cpuState]);
  return result;
}

unsigned long LinuxParser::IdleJiffies() {
  unsigned long result{0UL};
  auto cpuUtilization{CpuUtilization()};
  for (const auto& cpuState : {CPUStates::kIdle_, CPUStates::kIOwait_})
    result += std::stoul(cpuUtilization[cpuState]);
  return result;
}

std::vector<std::string> LinuxParser::CpuUtilization() {
  std::string line;
  std::ifstream proc_file{kProcDirectory + kStatFilename};
  if (proc_file.is_open()) {
    getline(proc_file, line);
    proc_file.close();
  }
  line.erase(std::unique(line.begin(), line.end(), BothCharsAre<' '>),
             line.end());  // eliminate duplicate spaces
  std::stringstream ss;
  ss << line;
  std::string cpu;
  ss >> cpu;  // skip the "cpu " string at the beginning of the line
  std::vector<std::string> jiffies;
  std::string jiffy;
  for (auto i = 0; i < 8; ++i) {
    ss >> jiffy;
    jiffies.push_back(jiffy);
  }
  return jiffies;
}

int LinuxParser::TotalProcesses() {
  int totalProcesses{0};
  std::ifstream proc_file{kProcDirectory + kStatFilename};
  if (proc_file.is_open()) {
    std::string line, token;
    while (getline(proc_file, line)) {
      std::stringstream ss{line};
      ss >> token;
      if (token == "processes") {
        ss >> totalProcesses;
        break;
      }
    }
    proc_file.close();
  }
  return totalProcesses;
}

int LinuxParser::RunningProcesses() {
  int runningProcesses{0};
  std::ifstream proc_file{kProcDirectory + kStatFilename};
  if (proc_file.is_open()) {
    std::string line, token;
    while (getline(proc_file, line)) {
      std::stringstream ss{line};
      ss >> token;
      if (token == "procs_running") {
        ss >> runningProcesses;
        break;
      }
    }
    proc_file.close();
  }
  return runningProcesses;
}

std::string LinuxParser::Command(int pid) {
  std::string command;
  std::ifstream cmdline_file{kProcDirectory + std::to_string(pid) +
                             kCmdlineFilename};
  if (cmdline_file.is_open()) {
    getline(cmdline_file, command);
    cmdline_file.close();
  }
  return command;
}

std::string LinuxParser::Ram(int pid) {
  unsigned long ram{0UL};
  std::ifstream status_file{kProcDirectory + std::to_string(pid) +
                            kStatusFilename};
  if (status_file.is_open()) {
    std::string line;
    std::string first_token;
    while (getline(status_file, line)) {
      line.erase(std::unique(line.begin(), line.end(), BothCharsAre<' '>),
                 line.end());  // eliminate duplicate spaces
      std::stringstream ss{line};
      ss >> first_token;
      if (first_token == "VmSize:") {
        ss >> ram;
        break;
      }
    }
    status_file.close();
  }
  return std::to_string(ram / 1024);  // convert from kB to MB
}

std::string LinuxParser::Uid(int pid) {
  std::string uid;
  std::ifstream status_file{kProcDirectory + std::to_string(pid) +
                            kStatusFilename};
  if (status_file.is_open()) {
    std::string line;
    std::string first_token;
    while (getline(status_file, line)) {
      std::replace(line.begin(), line.end(), '\t', ' ');
      line.erase(std::unique(line.begin(), line.end(), BothCharsAre<' '>),
                 line.end());  // eliminate duplicate spaces
      std::stringstream ss{line};
      ss >> first_token;
      if (first_token == "Uid:") {
        ss >> uid;
        break;
      }
    }
    status_file.close();
  }
  return uid;
}

std::string LinuxParser::User(int pid) {
  std::string username;
  std::string user_id{Uid(pid)};
  std::ifstream status_file{kPasswordPath};
  if (status_file.is_open()) {
    std::string line;
    std::string first_token;
    while (getline(status_file, line)) {
      std::size_t colon1_index{line.find_first_of(':', 0)};
      std::size_t colon2_index{line.find_first_of(':', colon1_index + 1)};
      std::size_t colon3_index{line.find_first_of(':', colon2_index + 1)};
      std::string parsed_user_id{
          line.substr(colon2_index + 1, colon3_index - (colon2_index + 1))};
      if (parsed_user_id == user_id) {
        username = line.substr(0, colon1_index);
        break;
      }
    }
    status_file.close();
  }
  return username;
}

// TODO: Read and return the uptime of a process
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::UpTime(int pid [[maybe_unused]]) { return 0; }
