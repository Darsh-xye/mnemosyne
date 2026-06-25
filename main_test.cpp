#include "allocator.h"
#include <iostream>

using namespace std;
using namespace ma;

struct Person {
    int age;

    Person(int a) : age(a) {
        cout << "Person constructed\n";
    }

    ~Person() {
        cout << "Person destroyed\n";
    }
};

int main() {
    init();

    void* p = alloc(20);
    free(p, 20);

    Person* x = make<Person>(21);
    cout << "Age: " << x->age << "\n";
    destroy(x);

    shutdown();
    return 0;
}