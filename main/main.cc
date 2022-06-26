#include "api.h"
#include "util.h"
#include <thread>
#include <chrono>
#include <vector>
#include <iostream>
#include <urcu.h>
#include <urcu/list.h>

DEFINE_SPINLOCK(mylock);

struct route_entry {
    struct cds_list_head re_next;
    unsigned long addr;
    unsigned long iface;
};
CDS_LIST_HEAD(route_list);

unsigned long route_lookup(unsigned long addr)
{
    struct route_entry *rep;
    unsigned long ret;

    cds_list_for_each_entry(rep, &route_list, re_next) {
        if (rep->addr == addr) {
            ret = rep->iface;
            return ret;
        }
    }
    return ULONG_MAX;
}

int route_add(unsigned long addr, unsigned long interface)
{
    struct route_entry *rep;

    rep = (route_entry*)malloc(sizeof(*rep));
    if (!rep)
        return -ENOMEM;
    rep->addr = addr;
    rep->iface = interface;
    cds_list_add(&rep->re_next, &route_list);
    return 0;
}

int route_del(unsigned long addr)
{
    struct route_entry *rep;

    cds_list_for_each_entry(rep, &route_list, re_next) {
        if (rep->addr == addr) {
            cds_list_del(&rep->re_next);
            free(rep);
            return 0;
        }
    }
    return -ENOENT;
}

void worker(int x, std::string str) {
    printf("worker %d started...\n", x);
    for (int i = 0; i < 100; ++i) {
        spin_lock(&mylock);
        printf("Hello World %d\n", x);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        spin_unlock(&mylock);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

/*
int main() {
    std::vector<std::thread> threads;
    for (int j = 0; j < 40; j++) {
        std::thread t(worker, j, std::to_string(j));
        threads.emplace_back(std::move(t));
    }
    for (auto& th : threads) {
        th.join();
    }
    return 0;
}
*/

int main() {
    route_add(1, 2);
    route_add(3, 4);
    route_add(5, 6);

    std::cout << "iface -> " << route_lookup(1) << std::endl;

    route_del(1);

    std::cout << "iface -> " << route_lookup(1) << std::endl;

    return 0;
}
