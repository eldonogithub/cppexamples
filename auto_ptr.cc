#include <memory>
#include <ostream>
#include <iostream>

class Base {
public:
  Base() {
    std::cout << *this << ": Constructed" << std::endl;
  }

  virtual ~Base() {
    std::cout << *this << ": Destructed" << std::endl;
  }

  friend std::ostream &operator<<(std::ostream &os, Base &obj) {
    os << "Base";
    return os;
  }
};

class Derived : public Base {
public:
  Derived() : Base() {
    std::cout << *this << ": Constructed" << std::endl;
  }
  virtual ~Derived() {
    std::cout << *this << ": Destructed" << std::endl;
  }
  friend std::ostream &operator<<(std::ostream &os, Derived &obj) {
    os << "Derived";
    return os;
  }
};

void manageTask(std::auto_ptr<Base> task ) {
  std::cout << __FUNCTION__ << " called" << std::endl;
  if ( task.get() != NULL ) {
    std::cout << ": task has a pointer to " << task.get() << std::endl;
  }
  else {
    std::cout << ": task doesn't have a pointer" << std::endl;
  }
  return;
}

int main(int argc, char *argv[])
{
  std::auto_ptr<Derived> p(new Derived());

  manageTask(static_cast<std::auto_ptr<Base> >(p));

  manageTask(std::auto_ptr<Base>(p));
  
  return 0;
}
