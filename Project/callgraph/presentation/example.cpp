#include <iostream>

using namespace std;

class A {
    public:
        int a;
        int PrintA ( void )
        {
            return a;
        }
};

class B1: virtual public A {

};

class B2: virtual public A {

};

class C: public B1, public B2 {

};

int main ( void )
{
    C c;

    c.B1::a = 1;
    c.B2::a = 2;
    c.B1::PrintA();
    c.B2::PrintA();
    
    return 0;
}