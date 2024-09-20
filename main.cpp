#include <iostream>
#include <array>

#include "SimInfo.h"
#include "Init.h"
#include "ComputeDt.h"
#include "IOManager.h"
#include "Update.h"

using namespace fv2d;

int main(int argc, char **argv) {
  // Initializing Kokkos
  Kokkos::initialize(argc, argv);
  {
    std::cout << "░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░" << std::endl;
    std::cout << "░        ░░   ░░░░░░░░░   ░░░░░░░░░░░░░░░░░   ░" << std::endl;
    std::cout << "▒   ▒▒▒▒▒▒▒▒   ▒▒▒▒▒▒▒   ▒▒▒   ▒  ▒▒▒▒▒▒▒▒▒   ▒" << std::endl;
    std::cout << "▒   ▒▒▒▒▒▒▒▒▒   ▒▒▒▒▒   ▒▒▒  ▒▒▒▒▒   ▒▒▒▒▒▒   ▒" << std::endl;
    std::cout << "▓       ▓▓▓▓▓▓   ▓▓▓   ▓▓▓▓▓▓▓▓▓   ▓▓▓▓   ▓   ▓" << std::endl;
    std::cout << "▓   ▓▓▓▓▓▓▓▓▓▓▓   ▓   ▓▓▓▓▓▓▓▓   ▓▓▓▓▓  ▓▓▓   ▓" << std::endl;
    std::cout << "▓   ▓▓▓▓▓▓▓▓▓▓▓▓     ▓▓▓▓▓▓▓   ▓▓▓▓▓▓▓  ▓▓▓   ▓" << std::endl;
    std::cout << "█   █████████████   ███████         ███   █   █" << std::endl;
    std::cout << "███████████████████████████████████████████████" << std::endl;

    // Reading parameters from .ini file
    auto params = readInifile(argv[1]);

    // Allocating main views
    Array U    = Kokkos::View<real_t***>("U",    params.Nty, params.Ntx, Nfields);
    Array Unew = Kokkos::View<real_t***>("Unew", params.Nty, params.Ntx, Nfields);
    Array Q    = Kokkos::View<real_t***>("Q",    params.Nty, params.Ntx, Nfields);


    // Misc vars for iteration
    real_t t = 0.0;
    int save_ite = 0;
    int ite = 0;
    // Initializing primitive variables
    InitFunctor init(params);
    UpdateFunctor update(params);
    ComputeDtFunctor computeDt(params);
    IOManager ioManager(params);

    init.init(Q);
    primToCons(Q, U, params);

    real_t dt;
    t = 0.0;
    real_t next_save = 0.0;
    int next_log = 0;

    while (t + params.epsilon < params.tend) {
      Kokkos::deep_copy(Unew, U);

      bool save_needed = (t + params.epsilon > next_save);

      consToPrim(U, Q, params);
      dt = computeDt.computeDt(Q, (ite == 0 ? params.save_freq : next_save-t), t, next_log == 0);
      if (next_log == 0)
        next_log = params.log_frequency;
      else
        next_log--;

      if (dt < 1.0e-10) {
        std::cout << "DT too small : Aborting (dt=" << dt << ")" << std::endl;
        break;
      }

      if (save_needed) {
        std::cout << " - Saving at time " << t << std::endl;
        ioManager.saveSolution(Q, save_ite++, t, dt);
        next_save += params.save_freq;

      }

      update.update(Q, Unew, dt, ite);

      Kokkos::deep_copy(U, Unew);

      t += dt;
      ite++;
    }

    ioManager.saveSolution(Q, save_ite++, t, dt);
  }
  Kokkos::finalize();

  std::cout << std::endl << std::endl;
  std::cout << "    █     ▀██  ▀██         ▀██                              ▄█▄ " << std::endl;
  std::cout << "   ███     ██   ██       ▄▄ ██    ▄▄▄   ▄▄ ▄▄▄     ▄▄▄▄     ███ " << std::endl;
  std::cout << "  █  ██    ██   ██     ▄▀  ▀██  ▄█  ▀█▄  ██  ██  ▄█▄▄▄██    ▀█▀ " << std::endl;
  std::cout << " ▄▀▀▀▀█▄   ██   ██     █▄   ██  ██   ██  ██  ██  ██          █  " << std::endl;
  std::cout << "▄█▄  ▄██▄ ▄██▄ ▄██▄    ▀█▄▄▀██▄  ▀█▄▄█▀ ▄██▄ ██▄  ▀█▄▄▄▀     ▄  " << std::endl;
  std::cout << "                                                            ▀█▀ " << std::endl;

  return 0;
}
