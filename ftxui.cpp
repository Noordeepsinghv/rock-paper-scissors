#include <ftxui/ftxui.hpp>
using namespace ftxui;

int main() {
  auto document =
    vbox({
      hbox({
        text("one") | border,
        text("two") | border | flex,
        text("three") | border | flex,
      }),

      gauge(0.25) | color(Color::Red),
      gauge(0.50) | color(Color::White),
      gauge(0.75) | color(Color::Blue),
    });

  auto screen = Screen::Create(Dimension::Full());
  Render(screen, document);
  screen.Print();

  return 0;
}