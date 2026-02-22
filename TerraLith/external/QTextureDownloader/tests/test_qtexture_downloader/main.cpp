/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <iostream>

#include <QApplication>

#include "qtexture_downloader.hpp"

int main(int argc, char *argv[])
{
  qtd::Logger::log()->info("testing qtexture_downloader...");

  qputenv("QT_LOGGING_RULES", QTD_QPUTENV_QT_LOGGING_RULES);
  QApplication app(argc, argv);

  // style
  const std::string style_sheet =
#include "darkstyle.css"
      ;
  app.setStyleSheet(style_sheet.c_str());

  // widget
  qtd::TextureDownloader *dw = new qtd::TextureDownloader("Test widget");
  dw->show();

  // output for dbg
  dw->connect(dw,
              &qtd::TextureDownloader::textures_retrieved,
              [](const std::vector<std::string> &paths)
              {
                std::cout << "Texture paths:\n";
                for (auto &s : paths)
                  std::cout << " - " << s << "\n";
              });

  return app.exec();
}
