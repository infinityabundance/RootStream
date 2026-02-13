#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <string>
#include <cstdint>
#include <ctime>

class DiskManager {
private:
    std::string output_directory;
    uint64_t max_storage_mb;
    uint32_t auto_cleanup_threshold_percent;
    
    struct {
        uint64_t total_space_mb;
        uint64_t free_space_mb;
        uint64_t used_space_mb;
    } disk_info;
    
public:
    DiskManager();
    ~DiskManager();
    
    // Initialization
    int init(const char *directory, uint64_t max_storage_mb);
    
    // Space management
    int refresh_disk_space();
    uint64_t get_free_space_mb();
    uint64_t get_used_space_mb();
    float get_usage_percent();
    
    // Cleanup
    int auto_cleanup_old_recordings();
    int remove_recording(const char *filename);
    int cleanup_directory();
    
    // Organization
    std::string generate_filename(const char *game_name = nullptr);
    const char* get_output_directory() { return output_directory.c_str(); }
    
    // Warnings
    bool is_space_low();
    bool is_at_limit();
    
    void cleanup();
};

#endif /* DISK_MANAGER_H */
