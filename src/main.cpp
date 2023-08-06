#include <slime/driver/driver.h>
#include <memory>

using slime::Driver;

int main(int argc, char* argv[]) {
    std::unique_ptr<Driver> driver{Driver::create()};
    driver->execute(argc - 1, argv + 1);
    return 0;
}
