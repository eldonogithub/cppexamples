#include <memory>
#include <iostream>

class A {
public:
  explicit A() : m_id(m_count++) { std::cout << *this << ": Constructed" << std::endl; }
  virtual ~A() { std::cout << *this << ": Destructed" << std::endl; }

  friend std::ostream &operator<<(std::ostream &out, A &obj) {
    return out << "A[" << obj.m_id << "]";
  }

private:
  static int m_count;
  int m_id;
};

int A::m_count = 0;

int main(int argc, char *argv[]) {
  std::unique_ptr<A> a(new A());
  std::cout << (bool)a  << std::endl;

  std::unique_ptr<A> b(new A());
  std::cout << b.operator bool() << std::endl;

  std::unique_ptr<A> c = std::move(a);

  std::shared_ptr<A> d = std::move(c);
  std::shared_ptr<A> e(b.release());
}
