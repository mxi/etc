#import <cstdio>
#import "iface.hpp"

void Platform(void)
{
    std::puts("Hello platform!");
}

int main(void)
{
    IF::DoSomething();
    return 0;
}
