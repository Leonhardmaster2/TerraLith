/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtd/delegates.hpp"

namespace qtd
{

ThumbnailDelegate::ThumbnailDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void ThumbnailDelegate::paint(QPainter                   *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex          &index) const
{
  painter->save();

  QPixmap pix = index.data(Qt::DecorationRole).value<QPixmap>();

  if (!pix.isNull())
  {
    QRect r = option.rect;
    r.adjust(2, 2, -2, -2);

    // scale to fill available rect, preserving aspect ratio
    QPixmap scaled = pix.scaled(r.size(),
                                Qt::KeepAspectRatio, // ByExpanding,
                                Qt::SmoothTransformation);

    // center inside rect if aspect doesn’t match
    QPoint topLeft(r.x() + (r.width() - scaled.width()) / 2,
                   r.y() + (r.height() - scaled.height()) / 2);

    painter->drawPixmap(topLeft, scaled);
  }

  painter->restore();
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem & /* option */,
                                  const QModelIndex &) const
{
  return QTD_CONFIG->widget.thumbnail_size;
}

} // namespace qtd
