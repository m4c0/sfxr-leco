export module lek;
import casein;
import quack;

export namespace lek {
constexpr const auto ddkpitch = 640;

unsigned *ddkscreen32;
int mouse_x, mouse_y, mouse_px, mouse_py;
bool mouse_left = false, mouse_right = false, mouse_middle = false;
bool mouse_leftclick = false, mouse_rightclick = false,
     mouse_middleclick = false;

void ddkLock();
void ddkUnlock();
void ddkSetMode(int width, int height, int bpp, int refreshrate, int fullscreen,
                const char *title);
void InitAudio();

class DPInput {
public:
  DPInput(int, int) {}
  static void Update() {}
  static bool KeyPressed(auto key) { return false; }
};
} // namespace lek

extern "C" bool ddkCalcFrame();

extern "C" void casein_handle(const casein::event &e) {
  static quack::renderer r{3};
  static quack::ilayout s{&r, 1};
  static quack::mouse_tracker mouse{};

  switch (e.type()) {
  case casein::TIMER:
    ddkCalcFrame();
    break;
  default:
    break;
  }

  r.process_event(e);
  s.process_event(e);
  mouse.process_event(e);
}
