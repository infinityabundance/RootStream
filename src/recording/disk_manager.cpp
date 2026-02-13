#include "disk_manager.h"
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

DiskManager::DiskManager() 
    : max_storage_mb(10000), auto_cleanup_threshold_percent(90) {
    disk_info.total_space_mb = 0;
    disk_info.free_space_mb = 0;
    disk_info.used_space_mb = 0;
}

DiskManager::~DiskManager() {
    cleanup();
}

int DiskManager::init(const char *directory, uint64_t max_storage_mb) {
    if (!directory) {
        fprintf(stderr, "ERROR: Invalid directory\n");
        return -1;
    }
    
    output_directory = directory;
    this->max_storage_mb = max_storage_mb;
    
    // Create directory if it doesn't exist
    struct stat st;
    if (stat(directory, &st) != 0) {
        if (mkdir(directory, 0755) != 0) {
            fprintf(stderr, "ERROR: Failed to create directory: %s\n", directory);
            return -1;
        }
    }
    
    // Refresh disk space info
    return refresh_disk_space();
}

int DiskManager::refresh_disk_space() {
    struct statvfs stat;
    
    if (statvfs(output_directory.c_str(), &stat) != 0) {
        fprintf(stderr, "ERROR: Failed to get disk space info\n");
        return -1;
    }
    
    uint64_t block_size = stat.f_frsize;
    disk_info.total_space_mb = (stat.f_blocks * block_size) / (1024 * 1024);
    disk_info.free_space_mb = (stat.f_bavail * block_size) / (1024 * 1024);
    disk_info.used_space_mb = disk_info.total_space_mb - disk_info.free_space_mb;
    
    return 0;
}

uint64_t DiskManager::get_free_space_mb() {
    refresh_disk_space();
    return disk_info.free_space_mb;
}

uint64_t DiskManager::get_used_space_mb() {
    refresh_disk_space();
    return disk_info.used_space_mb;
}

float DiskManager::get_usage_percent() {
    refresh_disk_space();
    if (disk_info.total_space_mb == 0) return 0.0f;
    return (float)disk_info.used_space_mb / disk_info.total_space_mb * 100.0f;
}

int DiskManager::auto_cleanup_old_recordings() {
    if (get_usage_percent() < auto_cleanup_threshold_percent) {
        return 0;  // No cleanup needed
    }
    
    // Get all recording files
    DIR *dir = opendir(output_directory.c_str());
    if (!dir) {
        fprintf(stderr, "ERROR: Cannot open directory: %s\n", output_directory.c_str());
        return -1;
    }
    
    struct FileInfo {
        std::string path;
        time_t mtime;
    };
    
    std::vector<FileInfo> files;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            std::string filepath = output_directory + "/" + entry->d_name;
            struct stat st;
            if (stat(filepath.c_str(), &st) == 0) {
                files.push_back({filepath, st.st_mtime});
            }
        }
    }
    closedir(dir);
    
    // Sort by modification time (oldest first)
    std::sort(files.begin(), files.end(), 
              [](const FileInfo &a, const FileInfo &b) {
                  return a.mtime < b.mtime;
              });
    
    // Remove oldest files until below threshold
    int removed_count = 0;
    for (const auto &file : files) {
        if (get_usage_percent() < auto_cleanup_threshold_percent * 0.8f) {
            break;  // Reduced to 80% of threshold
        }
        
        if (unlink(file.path.c_str()) == 0) {
            removed_count++;
            printf("INFO: Removed old recording: %s\n", file.path.c_str());
        }
    }
    
    return removed_count;
}

int DiskManager::remove_recording(const char *filename) {
    if (!filename) return -1;
    
    std::string filepath = output_directory + "/" + filename;
    if (unlink(filepath.c_str()) == 0) {
        return 0;
    }
    
    return -1;
}

int DiskManager::cleanup_directory() {
    // Remove all files in the directory
    DIR *dir = opendir(output_directory.c_str());
    if (!dir) return -1;
    
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            std::string filepath = output_directory + "/" + entry->d_name;
            if (unlink(filepath.c_str()) == 0) {
                count++;
            }
        }
    }
    closedir(dir);
    
    return count;
}

std::string DiskManager::generate_filename(const char *game_name) {
    time_t now = time(nullptr);
    struct tm *tm_info = localtime(&now);
    
    std::ostringstream oss;
    
    if (game_name && strlen(game_name) > 0) {
        oss << game_name << "_";
    } else {
        oss << "recording_";
    }
    
    oss << std::put_time(tm_info, "%Y%m%d_%H%M%S");
    oss << ".mp4";
    
    return oss.str();
}

bool DiskManager::is_space_low() {
    return get_free_space_mb() < 1000;  // Less than 1GB
}

bool DiskManager::is_at_limit() {
    refresh_disk_space();
    
    // Calculate used space in our output directory
    DIR *dir = opendir(output_directory.c_str());
    if (!dir) return false;
    
    uint64_t total_size = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            std::string filepath = output_directory + "/" + entry->d_name;
            struct stat st;
            if (stat(filepath.c_str(), &st) == 0) {
                total_size += st.st_size;
            }
        }
    }
    closedir(dir);
    
    uint64_t total_mb = total_size / (1024 * 1024);
    return total_mb >= max_storage_mb;
}

void DiskManager::cleanup() {
    // Nothing to cleanup for now
}
