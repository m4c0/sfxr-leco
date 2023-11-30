#pragma leco add_shader "lek.frag"
#pragma leco add_shader "lek.vert"
export module lek;
import casein;
import silog;
import vee;
import voo;

export namespace lek {
constexpr const auto ddkpitch = 640;

unsigned *ddkscreen32;
int mouse_x, mouse_y, mouse_px, mouse_py;
bool mouse_left = false, mouse_right = false, mouse_middle = false;
bool mouse_leftclick = false, mouse_rightclick = false,
     mouse_middleclick = false;

void ddkLock() {}
void ddkUnlock() {}
void ddkSetMode(int width, int height, int bpp, int refreshrate, int fullscreen,
                const char *title) {
  silog::assert(width == 640 && height == 480, "expecting 640x480 window");
  // init texture? pretend we care?
}
void InitAudio() {}

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
protected:
  void run() override {
    voo::device_and_queue dq{"sfxr", native_ptr()};
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

    auto cb = vee::allocate_primary_command_buffer(dq.command_pool());
    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      auto pl = vee::create_pipeline_layout({*dsl});
      auto gp = vee::create_graphics_pipeline({
          .pipeline_layout = *pl,
          .render_pass = sw.render_pass(),
          .shaders{
              voo::shader("lek.vert.spv").pipeline_vert_stage(),
              voo::shader("lek.frag.spv").pipeline_frag_stage(),
          },
          .bindings{quad.vertex_input_bind()},
          .attributes{quad.vertex_attribute(0)},
      });

      resized() = false;
      while (!interrupted() && !resized()) {
        sw.acquire_next_image();

        {
          auto m = img.mapmem();
          lek::ddkscreen32 = static_cast<unsigned *>(*m);
          ddkCalcFrame();
        }

        {
          voo::cmd_buf_one_time_submit pcb{cb};
          img.run(pcb);

          auto scb = sw.cmd_render_pass(cb);
          vee::cmd_bind_gr_pipeline(cb, *gp);
          vee::cmd_bind_descriptor_set(cb, *pl, 0, dset);
          quad.run(scb, 0);
        }

        sw.queue_submit(dq.queue(), cb);
        sw.queue_present(dq.queue());
      }

      vee::device_wait_idle();
    }

    ddkFree();
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static thread t{};
  t.handle(e);
}
