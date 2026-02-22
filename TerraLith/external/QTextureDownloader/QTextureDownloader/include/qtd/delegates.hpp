/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <QPainter>
#include <QStyledItemDelegate>

#include "qtd/config.hpp"

namespace qtd
{

class ThumbnailDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  explicit ThumbnailDelegate(QObject *parent = nullptr);

  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const override;

  void paint(QPainter                   *painter,
             const QStyleOptionViewItem &option,
             const QModelIndex          &index) const override;
};

} // namespace qtd