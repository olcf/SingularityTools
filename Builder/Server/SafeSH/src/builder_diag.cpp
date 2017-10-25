#include <iostream>

#include "resource_manager.h"
#include "build_queue.h"

using namespace builder;

// Print builder diagnostic information
int main(int argc, char **argv) {
    std::cout << "Resources free: "<< ResourceManager::get_count(ResourceManager::SlotStatus::free);
    std::cout << "Resources reserved: "<< ResourceManager::get_count(ResourceManager::SlotStatus::reserved);

    std::cout << "Resources running: "<< BuildQueue::get_count(BuildQueue::JobStatus ::running);
    std::cout << "Resources running: "<< BuildQueue::get_count(BuildQueue::JobStatus ::queued);
    std::cout << "Resources running: "<< BuildQueue::get_count(BuildQueue::JobStatus ::finished);
    std::cout << "Resources running: "<< BuildQueue::get_count(BuildQueue::JobStatus ::killed);

    return 0;
}