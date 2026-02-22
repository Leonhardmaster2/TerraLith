/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QUrl>

#include "qtd/logger.hpp"

namespace qtd
{

bool download_file(const std::string &url, const std::string &file_path, bool overwrite)
{
  QNetworkAccessManager manager;
  QNetworkRequest       request(QUrl(QString::fromStdString(url)));
  QNetworkReply        *reply = manager.get(request);

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec(); // wait until finished

  if (reply->error() != QNetworkReply::NoError)
  {
    Logger::log()->error("download_file: download error: {}", url);
    reply->deleteLater();
    return false;
  }

  QFileInfo file_info(QString::fromStdString(file_path));
  if (file_info.exists() && !overwrite)
  {
    Logger::log()->trace("download_file: file already exists, skipping: {}", file_path);
    reply->deleteLater();
    return false;
  }

  QFile file(QString::fromStdString(file_path));
  if (!file.open(QIODevice::WriteOnly))
  {
    Logger::log()->error("download_file: error writing file: {}", file_path);
    reply->deleteLater();
    return false;
  }

  file.write(reply->readAll());
  file.close();
  reply->deleteLater();
  return true;
}

} // namespace qtd
