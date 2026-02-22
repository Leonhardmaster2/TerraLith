/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <string>

#include <QCoreApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include <nlohmann/json.hpp>

#include "qtd/logger.hpp"

namespace qtd
{

// helper

template <typename T>
inline bool json_safe_get(const nlohmann::json &j, const std::string &key, T &value)
{
  if (j.contains(key))
  {
    value = j.at(key).get<T>();
    return true;
  }
  else
  {
    Logger::log()->error("json_safe_get: JSON parse error: unknown key {}", key);
    return false;
  }
}

// class

class JsonFetcher : public QObject
{
  Q_OBJECT
public:
  explicit JsonFetcher(QObject *parent = nullptr);

  nlohmann::json fetch_sync(const std::string &url, int timeout_ms = 5000);

signals:
  void finished(const nlohmann::json &j);
  void error_occurred(const QString &msg);

private:
  QNetworkAccessManager manager;
};

} // namespace qtd