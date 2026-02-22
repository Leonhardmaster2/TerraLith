/* Copyright (c) 2023 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
typedef unsigned int uint;

#include <QSurfaceFormat>

#include "hesiod/app/hesiod_application.hpp"
#include "hesiod/cli/batch_mode.hpp"
#include "hesiod/logger.hpp"

#if defined(DEBUG_BUILD)
#define HSD_RMODE "Debug"
#elif defined(RELEASE_BUILD)
#define HSD_RMODE "Release"
#else
#define HSD_RMODE "!!! UNDEFINED !!!"
#endif

int main(int argc, char *argv[])
{
  hesiod::Logger::log()->info("Welcome to Hesiod v{}.{}.{}!",
                              HESIOD_VERSION_MAJOR,
                              HESIOD_VERSION_MINOR,
                              HESIOD_VERSION_PATCH);

  hesiod::Logger::log()->info("Release mode: {}", std::string(HSD_RMODE));

  // ----------------------------------- OpenGL surface format
  // Must be set before QApplication is created.
  // macOS supports OpenGL up to 4.1 Core Profile only.
  {
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
#ifdef HSD_OS_MACOS
    format.setVersion(4, 1);
#else
    format.setVersion(4, 3);
#endif
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);
  }

  // ----------------------------------- initialization

  // Enable HiDPI / Retina support (Qt6 handles this automatically, but
  // setting the environment variable ensures consistent behavior on macOS)
#ifdef HSD_OS_MACOS
  qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
#endif

  // start QApplication even if headless (for QObject)
  qputenv("QT_LOGGING_RULES", HESIOD_QPUTENV_QT_LOGGING_RULES);
  hesiod::HesiodApplication app(argc, argv);

  // ----------------------------------- batch CLI mode

  args::ArgumentParser parser("Hesiod.");
  int                  ret = hesiod::cli::parse_args(parser, argc, argv);

  if (ret >= 0)
    return ret;

  app.show();

  return app.exec();
}
