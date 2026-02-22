#pragma once
#include <QDebug>
#include <QEventLoop>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QTimer>
#include <QUrl>

namespace qtd
{

class ImageFetcher : public QObject
{
  Q_OBJECT
public:
  explicit ImageFetcher(QObject *parent = nullptr);

  QImage fetch_sync(const std::string &url, int timeout_ms = 5000);

signals:
  void finished(const QImage &image);
  void error_occurred(const QString &msg);

private:
  QNetworkAccessManager manager;
};

} // namespace qtd
