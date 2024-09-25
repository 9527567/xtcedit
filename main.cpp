#include "CLI11.hpp"
#include "xdrfile.h"
#include "xdrfile_xtc.h"
#include <cstdlib>
#include <filesystem>
#include <print>
#include <vector>
#include "tqdm.h"
int main(int argc, char **argv) {
  std::string logo = "\
,---.   ,--.,--------. ,-----. \n\
 \\  `.'  / '--.  .--''  .--./ \n\
  .'    \\     |  |   |  |     轨迹修改器 \n\
 /  .'.  \\    |  |   '  '--'\\ \n\
'--'   '--'   `--'    `-----'";
  std::print("{}\n", logo);
  CLI::App app{""};
  argv = app.ensure_utf8(argv);

  std::string filename = "default";
  std::string outfile_name = "output.xtc";
  app.add_option("-f,--file", filename, "输入轨迹文件。");
  app.add_option("-o,--outfile", outfile_name, "输出轨迹文件。");

  CLI11_PARSE(app, argc, argv);
  std::print("要修改的轨迹：{}\n", filename);
  if (std::filesystem::exists(filename)) {
    // 记录读取或者写入的状态
    int result = 0;
    // 文件描述符
    // 总原子数
    int natoms = 0;
    // 步数
    int step = 0;
    // 时间
    float time = 0;
    // 盒子定义 float[DIM][DIM]
    // 坐标 natoms * DIM
    float prec = 1000;
    int k = 0;
    rvec *x;
    matrix box;
    read_xtc_natoms(filename.c_str(), &natoms);
    std::vector<float> times;
    std::vector<int> steps;

    XDRFILE *xd_open = xdrfile_open(filename.c_str(), "r");
    std::println("打开文件");
    unsigned long nframes = 0;
    int64_t* offsets;
    read_xtc_header(filename.c_str(),&natoms,&nframes, &offsets);
    do {
      // 偏移量offset 代表跳过一帧
      result = read_times(xd_open, natoms, &step, &time, offsets[k+1]);
      if (exdrENDOFFILE != result) {
        times.push_back(time);
        steps.push_back(step);
        // }
      }
      k++;
    } while (result == exdrOK);
    free(offsets);
    std::println("轨迹总原子数：{}", natoms);
    std::println("总帧数：{}", steps.size());
    std::println("轨迹开始数：{}", steps[0]);
    std::println("模拟时间：从{} 纳秒到 {} 纳秒", (*times.begin()) / 1000,
                 (*(times.end() - 1)) / 1000);
    std::unordered_map<int, int> dstep;
    std::unordered_map<float, int> dtime;
    for (auto it = steps.begin(); it != steps.end() - 1; ++it) {
      dstep[*(it + 1) - *(it)]++;
    }
    for (auto it = times.begin(); it != times.end() - 1; ++it) {
      dtime[*(it + 1) - *(it)]++;
    }
    if (dstep.size() > 1) {
      std::println("警告：所有帧之间步长不唯一：{}。", dstep.size());
      for (auto &[fst, snd] : dstep) {
        std::println("步长{} 有 {}个。", fst, snd);
      }
    } else {
      std::println("{} 帧之间的步长均为 {}", steps.size(),
                   dstep.begin()->first);
    }
    if (dtime.size() > 1) {
      std::println("警告：所有时间间隔不唯一：{}。", dstep.size());
      for (auto &[fst, snd] : dtime) {
        std::println("时间间隔{} 有 {}个。", fst, snd);
      }
    } else {
      std::println("{} 时间间隔均为 {} 飞秒", times.size(),
                   dtime.begin()->first);
    }
    float begin_time = 0;
    int begin_frame = 0;
    float time_step = 0;
    int traj_step = 5000;
    std::println("输入开始的时间，单位是飞秒，例如从 0.1纳秒开始，那么就输入 "
                 "0.1 * 1000，也就是 100， 默认为 0。");
    std::cin >> begin_time;
    std::println("输入时间间隔，单位是飞秒，例如间隔为 0.1纳秒，那么就输入 0.1 "
                 "* 1000，也就是 100， 默认为 100飞秒。");
    std::cin >> time_step;
    std::println("输入轨迹开始数，例如 5000，默认为0。");
    std::cin >> begin_frame;
    std::println("输入轨迹间隔，默认为 5000。");
    std::cin >> traj_step;
    xdr_seek(xd_open, 0L, SEEK_SET);
    XDRFILE *xd_write = xdrfile_open(outfile_name.c_str(), "w");
    tqdm bar;
    for (k = 0; k < steps.size(); k++) {
      x = static_cast<rvec *>(calloc(natoms, sizeof(x[0])));
      read_xtc(xd_open, natoms, &step, &time, box, x, &prec);
      write_xtc(xd_write, natoms, begin_frame + traj_step * k,
                begin_time + time_step * static_cast<float>(k), box, x, prec);
      bar.progress(k, steps.size());
      free(x);
    }
    bar.finish();
    xdrfile_close(xd_open);
    xdrfile_close(xd_write);
    std::println("修改完成，轨迹保存在{}\n", outfile_name);
  } else {
    std::print("轨迹文件 {} 不存在\n", filename);
  }
  return 0;
}
