/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtd/json_fetcher.hpp"
#include "qtd/config.hpp"
#include "qtd/logger.hpp"

namespace qtd
{

JsonFetcher::JsonFetcher(QObject *parent) : QObject(parent) {}

nlohmann::json JsonFetcher::fetch_sync(const std::string &url, int timeout_ms)
{
  QNetworkRequest request(QUrl(QString::fromStdString(url)));
  QNetworkReply  *reply = manager.get(request);

  QEventLoop loop;

  // stop loop when finished
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

  // optional timeout
  QTimer timer;
  timer.setSingleShot(true);
  timer.start(timeout_ms);
  QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  loop.exec();

  if (!timer.isActive())
  {
    // timeout
    reply->abort();
    reply->deleteLater();
    return {};
  }

  timer.stop();

  QByteArray data = reply->readAll();
  reply->deleteLater();

  try
  {
    return nlohmann::json::parse(data.constData());
  }
  catch (const nlohmann::json::parse_error &e)
  {
    return {};
  }
}

} // namespace qtd
