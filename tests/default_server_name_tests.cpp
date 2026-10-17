#include "../src/DefaultServerName.h"
#include <cassert>

int main()
{
    std::string name;
    assert(BuildDefaultServerName("working", name));
    assert("working's Server" == name);
    assert(BuildDefaultServerName("\xe4\xb8\xad\xe6\x96\x87working", name));
    assert("\xe4\xb8\xad\xe6\x96\x87working's Server" == name);
    assert(BuildDefaultServerName("\xe4\xb8\xad\xe6\x96\x87", name));
    assert("\xe4\xb8\xad\xe6\x96\x87's Server" == name);
    assert(BuildDefaultServerName("caf\xc3\xa9\xf0\x9f\x98\x80", name));
    assert("caf\xc3\xa9\xf0\x9f\x98\x80's Server" == name);
    assert(BuildDefaultServerName("working   ", name));
    assert("working's Server" == name);
    assert(BuildDefaultServerName("", name));
    assert("Someone's Server" == name);
    assert(BuildDefaultServerName("   ", name));
    assert("Someone's Server" == name);
    assert(!BuildDefaultServerName(nullptr, name));
    assert(name.empty());
    assert(!BuildDefaultServerName("bad\xe4\xb8", name));
    assert(name.empty());

    //The suffix is nine bytes, leaving 54 bytes of the native 63-byte payload.
    assert(BuildDefaultServerName(std::string(54, 'a').c_str(), name));
    assert(std::string(54, 'a') + "'s Server" == name);
    assert(63 == name.size());
    assert(BuildDefaultServerName((std::string(53, 'a') + "\xe4\xb8\xad").c_str(), name));
    assert(std::string(53, 'a') + "'s Server" == name);
    assert(BuildDefaultServerName((std::string(51, 'a') + "\xe4\xb8\xad\xe6\x96\x87").c_str(), name));
    assert(std::string(51, 'a') + "\xe4\xb8\xad's Server" == name);
    assert(BuildDefaultServerName((std::string(51, 'a') + "\xf0\x9f\x98\x80").c_str(), name));
    assert(std::string(51, 'a') + "'s Server" == name);
    assert(BuildDefaultServerName((std::string(50, 'a') + "\xf0\x9f\x98\x80").c_str(), name));
    assert(std::string(50, 'a') + "\xf0\x9f\x98\x80's Server" == name);
    assert(BuildDefaultServerName(std::string(1000, 'a').c_str(), name));
    assert(std::string(54, 'a') + "'s Server" == name);
    assert(!BuildDefaultServerName((std::string(1000, 'a') + "\xff").c_str(), name));
    assert(name.empty());
}
