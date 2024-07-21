#pragma leco add_shader "lek.frag"
#pragma leco add_shader "lek.vert"
#pragma leco add_resource "font.tga"
#pragma leco add_resource "ld48.tga"
export module lek;
import casein;
import siaudio;
import silog;
import vee;
import voo;

extern "C" void audio_callback(float *buf, int size);
static void fill_buffer(float *f, unsigned num_samples) {
  audio_callback(f, num_samples);
}

export namespace lek {
constexpr const auto ddkpitch = 640;

unsigned ddkscreen32[640 * 480];
int mouse_x, mouse_y, mouse_px, mouse_py;
bool mouse_left = false, mouse_right = false, mouse_middle = false;
bool mouse_leftclick = false, mouse_rightclick = false,
     mouse_middleclick = false;

bool redrawing = false;
void ddkLock() { redrawing = true; }
void ddkUnlock() {}
void ddkSetMode(int width, int height, int bpp, int refreshrate, int fullscreen,
                const char *title) {
  silog::assert(width == 640 && height == 480, "expecting 640x480 window");
  // init texture? pretend we care?
}

void InitAudio() {
  siaudio::filler(fill_buffer);
  siaudio::rate(44100);
}

class DPInput {
public:
  DPInput(int, int) {}
  static void Update() {}
  static bool KeyPressed(auto key) { return false; }
};
} // namespace lek

extern "C" void ddkInit();
extern "C" bool ddkCalcFrame();
extern "C" void ddkFree();

class thread : public voo::casein_thread {
  void run() override {
    voo::device_and_queue dq{"sfxr"};
    voo::one_quad quad{dq.physical_device()};

    vee::descriptor_set_layout dsl =
        vee::create_descriptor_set_layout({vee::dsl_fragment_sampler()});
    vee::descriptor_pool dp =
        vee::create_descriptor_pool(1, {vee::combined_image_sampler(1)});
    vee::descriptor_set dset = vee::allocate_descriptor_set(*dp, *dsl);

    vee::sampler smp = vee::create_sampler(vee::nearest_sampler);

    voo::h2l_image img{dq.physical_device(), 640, 480};
    vee::update_descriptor_set(dset, 0, img.iv(), *smp);

    ddkInit();

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      auto pl = vee::create_pipeline_layout({*dsl});
      auto gp = vee::create_graphics_pipeline({
          .pipeline_layout = *pl,
          .render_pass = dq.render_pass(),
          .shaders{
              voo::shader("lek.vert.spv").pipeline_vert_stage(),
              voo::shader("lek.frag.spv").pipeline_frag_stage(),
          },
          .bindings{quad.vertex_input_bind()},
          .attributes{quad.vertex_attribute(0)},
      });

      extent_loop(dq.queue(), sw, [&] {
        {
          lek::redrawing = false;
          ddkCalcFrame();
          lek::mouse_leftclick = false;
          if (lek::redrawing) {
            auto m = voo::mapmem(img.host_memory());
            auto mm = static_cast<unsigned *>(*m);
            for (auto x = 0; x < 480 * 640; x++)
              mm[x] = lek::ddkscreen32[x] | 0xff000000;
          }
        }

        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          img.setup_copy(*pcb);

          auto scb = sw.cmd_render_pass(pcb);
          vee::cmd_set_viewport(*scb, sw.extent());
          vee::cmd_set_scissor(*scb, sw.extent());
          vee::cmd_bind_gr_pipeline(*scb, *gp);
          vee::cmd_bind_descriptor_set(*scb, *pl, 0, dset);
          quad.run(*scb, 0);
        });
      });
    }

    ddkFree();
  }
};

struct init {
  init() {
    using namespace casein;

    handle(MOUSE_DOWN, [] {
      lek::mouse_left = true;
      lek::mouse_leftclick = true;
    });
    handle(MOUSE_UP, [] { lek::mouse_left = false; });
    handle(MOUSE_MOVE, [] {
      using namespace lek;

      mouse_px = mouse_x;
      mouse_x = casein::mouse_pos.x * 640 / casein::window_size.x;

      mouse_py = mouse_y;
      mouse_y = casein::mouse_pos.y * 480 / casein::window_size.y;
    });

    static thread t{};
  }
} i;
