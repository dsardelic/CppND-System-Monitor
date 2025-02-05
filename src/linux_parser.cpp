#include "linux_parser.h"

#include <dirent.h>  // DIR, opendir
#include <unistd.h>  // sysconf

#include <algorithm>  // std::unique
#include <fstream>    // std::ifstream
#include <sstream>    // std::istringstream, std::stringstream
#include <string>     // std::string, std::stol
#include <vector>     // std::vector

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
  unsigned long memTotal{}, memAvailable{};
  std::ifstream meminfo_file{kProcDirectory + kMeminfoFilename};
  if (meminfo_file.is_open()) {
    bool memTotalFound{false}, memAvailableFound{false};
    std::string line;
    std::string first_token;
    while (getline(meminfo_file, line) &&
           !(memTotalFound && memAvailableFound)) {
      line.erase(std::unique(line.begin(), line.end(), BothCharsAre<' '>),
                 line.end());  // eliminate duplicate spaces
      std::stringstream ss{line};
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
  long upTime{};
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

unsigned long LinuxParser::ActiveJiffies(int pid) {
  unsigned long activeJiffies{};
  std::ifstream stat_file{kProcDirectory + std::to_string(pid) + kStatFilename};
  if (stat_file.is_open()) {
    std::string line;
    getline(stat_file, line);
    std::stringstream ss{line};
    std::string foo;
    for (auto i = 0; i < 13; ++i) {
      ss >> foo;
    }
    unsigned long utime;
    ss >> utime;
    unsigned long stime;
    ss >> stime;
    activeJiffies = utime + stime;
    stat_file.close();
  }
  return activeJiffies;
}

unsigned long LinuxParser::ActiveJiffies() {
  unsigned long result{};
  auto cpuUtilization{CpuUtilization()};
  for (const auto& cpuState :
       {CPUStates::kUser_, CPUStates::kNice_, CPUStates::kSystem_,
        CPUStates::kIRQ_, CPUStates::kSoftIRQ_, CPUStates::kSteal_})
    result += std::stoul(cpuUtilization[cpuState]);
  return result;
}

unsigned long LinuxParser::IdleJiffies() {
  unsigned long result{};
  auto cpuUtilization{CpuUtilization()};
  for (const auto& cpuState : {CPUStates::kIdle_, CPUStates::kIOwait_})
    result += std::stoul(cpuUtilization[cpuState]);
  return result;
}

std::vector<std::string> LinuxParser::CpuUtilization() {
  std::vector<std::string> jiffies = {"0", "0", "0", "1", "0", "0", "0", "0"};
  std::ifstream proc_file{kProcDirectory + kStatFilename};
  std::string line;
  if (proc_file.is_open()) {
    jiffies.clear();
    getline(proc_file, line);
    line.erase(std::unique(line.begin(), line.end(), BothCharsAre<' '>),
               line.end());  // eliminate duplicate spaces
    std::stringstream ss{line};
    std::string cpu;
    ss >> cpu;  // skip the "cpu " string at the beginning of the line
    std::string jiffy;
    const auto cpuStates = {CPUStates::kUser_,    CPUStates::kNice_,
                            CPUStates::kSystem_,  CPUStates::kIdle_,
                            CPUStates::kIOwait_,  CPUStates::kIRQ_,
                            CPUStates::kSoftIRQ_, CPUStates::kSteal_};
    for (std::size_t i = 0UL; i < cpuStates.size(); ++i) {
      ss >> jiffy;
      jiffies.push_back(jiffy);
    }
    proc_file.close();
  }
  return jiffies;
}

int LinuxParser::TotalProcesses() {
  int totalProcesses{};
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
  int runningProcesses{};
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
  unsigned long ram{};
  std::ifstream status_file{kProcDirectory + std::to_string(pid) +
                            kStatusFilename};
  if (status_file.is_open()) {
    std::string line, first_token;
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
    std::string line, first_token;
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
  std::ifstream status_file{kPasswordPath};
  if (status_file.is_open()) {
    std::string user_id{Uid(pid)}, line, first_token;
    while (getline(status_file, line)) {
      auto colon1_index{line.find_first_of(':', 0)};
      auto colon2_index{line.find_first_of(':', colon1_index + 1)};
      auto colon3_index{line.find_first_of(':', colon2_index + 1)};
      auto parsed_user_id{
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

long LinuxParser::UpTime(int pid) {
  long upTime{};
  std::ifstream stat_file{kProcDirectory + std::to_string(pid) + kStatFilename};
  if (stat_file.is_open()) {
    std::string line;
    getline(stat_file, line);
    std::stringstream ss{line};
    std::string foo;
    for (auto i = 0; i < 21; ++i) {
      ss >> foo;
    }
    long jiffies_at_start;
    ss >> jiffies_at_start;
    upTime = UpTime() - jiffies_at_start / sysconf(_SC_CLK_TCK);
    stat_file.close();
  }
  return upTime;
}
