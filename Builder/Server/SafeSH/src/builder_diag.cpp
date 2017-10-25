#include <iostream>

#include "resource_manager.h"
#include "build_queue.h"

using namespace builder;

// Print builder diagnostic information
int main(int argc, char **argv) {
    std::cout << "Resources free: "<< ResourceManager::get_count(ResourceManager::SlotStatus::free)<<std::endl;
    std::cout << "Resources reserved: "<< ResourceManager::get_count(ResourceManager::SlotStatus::reserved)<<std::endl;

    std::cout << "Builds running: "<< BuildQueue::get_count(BuildQueue::JobStatus ::running)<<std::endl;
    std::cout << "Builds queued: "<< BuildQueue::get_count(BuildQueue::JobStatus ::queued)<<std::endl;
    std::cout << "Builds finished: "<< BuildQueue::get_count(BuildQueue::JobStatus ::finished)<<std::endl;
    std::cout << "Builds killed: "<< BuildQueue::get_count(BuildQueue::JobStatus ::killed)<<std::endl;

    return 0;
}
