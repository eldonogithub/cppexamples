#include <string>
#include <memory>
#include <assert.h>

int main()
{
  {
    std::string a;

    for ( int i = 0; i < 1000; i++) {
      a.append("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    }

    const char *p_c_str = a.c_str();
    const char *p_data = a.data();

    assert(p_c_str == p_data);
  }

  {
    std::unique_ptr<std::string> a(new std::string("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));

    const char *p_c_str = a->c_str();
    const char *p_data = a->data();

    assert(p_c_str == p_data);
  }
}
