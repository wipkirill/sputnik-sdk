#include <mutex>
#include <vector>
#include <atomic>
#include <thread>
#include <cassert>
#include <iostream>
#include <condition_variable>

#include <UrbanLabs/Sdk/Concurrent/ReadWriteLock.h>

class Object : public ReadWriteLock {
public:
    void read() {
        ReadWriteLock::ReadLock lock(this);
        std::cout << "read\n";
    }

    void write() {
        ReadWriteLock::WriteLock lock(this);
        std::cout << "write\n";
    }
};

int main() {
    Object obj;
    int num = 10000;
    std::vector<std::thread> read(num);
    for(int i = 0; i < num; i++)
        read[i] = std::thread(&Object::read, &obj);

    std::vector<std::thread> write(num);
    for(int i = 0; i < num; i++)
        write[i] = std::thread(&Object::write, &obj);

    for(int i = 0; i < num; i++)
        read[i].join();

    for(int i = 0; i < num; i++)
        write[i].join();
    return 0;
}
