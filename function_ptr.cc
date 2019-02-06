#include <memory>
#include <iostream>

void foo() {
}


class Base {
  static int id;
  
  int m_a;
  int m_b;
  int m_id;

public:

  Base(int a, int b) : m_a(a), m_b(b), m_id(id++) {
    std::cout << *this << ": Constructed" << std::endl;
  }

  virtual ~Base() {
    std::cout << *this << ": Destroyed" << std::endl;
  }

  void method() {
    std::cout << *this << ": Method()" << std::endl;
  }

  void operator()() {
    std::cout << *this << ": operator(" << m_a << "," << m_b << ")" << std::endl;
  }

  friend std::ostream &operator<<(std::ostream &os, Base &obj) {
    return os << "Base[" << obj.m_id << "]";
  }
};

int Base::id = 0;

int main() {
  std::auto_ptr<Base> p(new Base(7,42));

  foo(p.get());

  void (*p)();

  return 0;
}
