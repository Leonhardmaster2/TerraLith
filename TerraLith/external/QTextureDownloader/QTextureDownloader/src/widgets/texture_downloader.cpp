/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <filesystem>

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>

#include "qtd/config.hpp"
#include "qtd/delegates.hpp"
#include "qtd/logger.hpp"
#include "qtd/texture_downloader.hpp"
#include "qtd/utils.hpp"

namespace qtd
{

TextureDownloader::TextureDownloader(const std::string &_title, QWidget *parent)
    : QWidget(parent), title(_title)
{
  Logger::log()->trace("TextureDownloader::TextureDownloader");

  this->setWindowTitle(this->title.c_str());
  this->setFocusPolicy(Qt::StrongFocus);
  this->setMouseTracking(true);
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  this->setup_layout();
  this->setup_menu_bar();
  this->setup_connections();

  this->texture_manager.load();
  this->update_table_rows();
}

TextureDownloader::~TextureDownloader()
{
  Logger::log()->trace("TextureDownloader::~TextureDownloader");
  this->texture_manager.save();
}

void TextureDownloader::choose_storage_path()
{
  QString dir = QFileDialog::getExistingDirectory(
      this,
      tr("Select Storage Directory"),
      QString::fromStdString(this->texture_manager.get_storage_path()),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (!dir.isEmpty())
  {
    this->texture_manager.set_storage_path(dir.toStdString());
    this->update_table_rows();

    Logger::log()->info("Storage path set to: {}",
                        this->texture_manager.get_storage_path());
  }
}

void TextureDownloader::closeEvent(QCloseEvent *event)
{
  this->texture_manager.save();
  event->accept();
  Q_EMIT this->window_closed();
}

void TextureDownloader::purge_database()
{
  Logger::log()->trace("TextureDownloader::purge_database");

  auto reply = QMessageBox::question(
      this,
      tr("Purge Database"),
      tr("This process will remove all local data, including textures image files.\nAre "
         "you sure you want to continue?"),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);

  if (reply != QMessageBox::Yes)
  {
    Logger::log()->info("TextureDownloader::purge_database cancelled by user");
    return;
  }

  // block UI
  QProgressDialog progress(tr("Updating sources..."), QString(), 0, 0, this);
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setCancelButton(nullptr);
  progress.setMinimumDuration(0); // show immediately
  progress.show();

  // process events so dialog is painted before heavy work
  QApplication::processEvents();

  // TODO remove files

  this->texture_manager = TextureManager();
  this->texture_manager.save();
  this->update_table_rows();

  progress.close();
}

void TextureDownloader::retrieve_selected_textures()
{
  Logger::log()->trace("TextureDownloader::retrieve_selected_textures");

  std::vector<std::string> texture_paths;

  // block UI
  QProgressDialog progress(tr("Retrieving textures..."), QString(), 0, 0, this);
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setCancelButton(nullptr);
  progress.setMinimumDuration(0); // show immediately
  progress.show();

  // process events so dialog is painted before heavy work
  QApplication::processEvents();

  // list checked items

  for (int row = 0; row < this->table_model->rowCount(); ++row)
    for (int col = 0; col < this->table_model->columnCount(); ++col)
    {
      QStandardItem *item = this->table_model->item(row, col);

      // TODO hardcoded stuffs...

      // avoid col == 1: is_pinned
      if (item && item->isCheckable() && item->checkState() == Qt::Checked && col != 1)
      {
        TextureType type;

        // TODO hardcoded stuffs...

        // which type?
        if (col == 5)
          type = TextureType::DIFFUSE;
        else if (col == 6)
          type = TextureType::NORMAL;
        else
          type = TextureType::DISPLACEMENT;

        // retrieve ID
        QStandardItem *item_id = this->table_model->item(row, 2); // TODO hardcoded
        TextureKey     key(item_id->text().toStdString(), type, this->res);
        std::string    path = this->texture_manager.try_download_texture(key);

        if (!path.empty())
          texture_paths.push_back(path);
      }
    }

  // free selection
  this->unchecked_all_items();

  // unblock UI
  progress.close();

  Q_EMIT this->textures_retrieved(texture_paths);
}

void TextureDownloader::set_texture_res(const TextureRes &new_res)
{
  Logger::log()->trace("TextureDownloader::set_texture_res");

  if (new_res == TextureRes::RUNKNOWN || new_res == this->res)
    return;

  this->res = new_res;
  this->update_table_rows();
  this->table_view->sortByColumn(1, Qt::DescendingOrder);
}

void TextureDownloader::setup_connections()
{
  Logger::log()->trace("TextureDownloader::setup_connections");

  this->connect(this->button_get_selected,
                &QPushButton::clicked,
                this,
                &TextureDownloader::retrieve_selected_textures);

  this->connect(this->button_uncheck_items,
                &QPushButton::clicked,
                this,
                &TextureDownloader::unchecked_all_items);

  this->connect(this->combo_res,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this]()
                {
                  std::string res_name = this->combo_res->currentText().toStdString();
                  TextureRes  new_res = TextureRes::RUNKNOWN;
                  for (auto &[r, str] : texture_res_as_string)
                  {
                    if (str == res_name)
                    {
                      new_res = r;
                      break;
                    }
                  }
                  this->set_texture_res(new_res);
                });

  this->connect(
      this->table_model,
      &QStandardItemModel::itemChanged,
      this,
      [this](QStandardItem *item)
      {
        if (item->isCheckable())
        {
          item->setText(item->checkState() == Qt::Checked ? "Y" : "N");

          int col = item->index().column();

          if (col == 1)
          {
            int row = item->index().row();

            // TODO hardcoded
            std::string tex_id = this->table_model->item(row, 2)->text().toStdString();
            this->texture_manager.get_textures().at(tex_id).set_is_pinned(
                item->checkState());
          }
        }
      });
}

void TextureDownloader::setup_layout()
{
  Logger::log()->trace("TextureDownloader::setup_layout");

  // create and assign new layout
  QGridLayout *layout = new QGridLayout(this);
  layout->setContentsMargins(2, 0, 2, 0);
  this->setLayout(layout);

  /// --- first line

  int col = 0;

  // get textures
  this->button_get_selected = new QPushButton("Get selected");
  layout->addWidget(this->button_get_selected, 0, col++);

  // clear checked items
  this->button_uncheck_items = new QPushButton("Uncheck items");
  layout->addWidget(this->button_uncheck_items, 0, col++);

  // combo
  this->combo_res = new QComboBox();
  layout->addWidget(this->combo_res, 0, col++);

  for (auto &r : all_texture_res)
  {
    std::string res_name = texture_res_as_string.at(r);
    this->combo_res->addItem(res_name.c_str());
  }

  std::string current_res_str = texture_res_as_string.at(this->res);
  int         idx = this->combo_res->findText(current_res_str.c_str());
  if (idx >= 0)
    this->combo_res->setCurrentIndex(idx);

  // --- table

  this->table_model = new QStandardItemModel(0, 0, this);

  // labels
  QStringList labels = {"Thumbnail", "Pinned", "ID", "Name", "Source"};
  for (auto &r : all_texture_types)
  {
    std::string type_name = texture_type_as_string.at(r);
    labels << type_name.c_str();
  }

  this->table_model->setHorizontalHeaderLabels(labels);

  this->table_view = new QTableView(this);
  this->table_view->setModel(this->table_model);
  this->table_view->setSortingEnabled(true);
  this->table_view->horizontalHeader()->setStretchLastSection(true);
  this->table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  this->table_view->setItemDelegateForColumn(0, new ThumbnailDelegate(this->table_view));

  this->table_view->horizontalHeader()->setSectionResizeMode(
      0,
      QHeaderView::ResizeToContents);
  this->table_view->verticalHeader()->setDefaultSectionSize(
      QTD_CONFIG->widget.thumbnail_size.height());

  layout->addWidget(this->table_view, 1, 0, 1, 4);
}

void TextureDownloader::setup_menu_bar()
{
  Logger::log()->trace("TextureDownloader::setup_menu_bar");

  QMenuBar *menu_bar = new QMenuBar(this);
  QMenu    *file_menu = menu_bar->addMenu("Texture sources");

  // update
  {
    QAction *action = new QAction(tr("&Update sources"), this);
    file_menu->addAction(action);

    this->connect(action, &QAction::triggered, this, &TextureDownloader::update_sources);
  }

  // storage path
  {
    QAction *action = new QAction(tr("&Change storage path"), this);
    file_menu->addAction(action);

    this->connect(action,
                  &QAction::triggered,
                  this,
                  &TextureDownloader::choose_storage_path);
  }

  file_menu->addSeparator();

  // purge
  {
    QAction *action = new QAction(tr("&Purge database"), this);
    file_menu->addAction(action);

    this->connect(action, &QAction::triggered, this, &TextureDownloader::purge_database);
  }

  QLayout *layout = this->layout();
  if (layout)
    layout->setMenuBar(menu_bar);
}

QSize TextureDownloader::sizeHint() const { return QSize(QTD_CONFIG->widget.size_hint); }

void TextureDownloader::unchecked_all_items()
{
  for (int row = 0; row < this->table_model->rowCount(); ++row)
    for (int col = 0; col < this->table_model->columnCount(); ++col)
    {
      if (col != 1) // is_pinned
      {
        QStandardItem *item = this->table_model->item(row, col);
        if (item && item->isCheckable())
        {
          item->setCheckState(Qt::Unchecked);
        }
      }
    }
}

void TextureDownloader::update_sources()
{
  Logger::log()->trace("TextureDownloader::update_sources");

  auto reply = QMessageBox::question(
      this,
      tr("Update Sources"),
      tr("Retrieving textures metadata, this process may take a while.\nAre you sure you "
         "want to continue?"),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);

  if (reply != QMessageBox::Yes)
  {
    Logger::log()->info("TextureDownloader::update_sources cancelled by user");
    return;
  }

  // block UI
  QProgressDialog progress(tr("Updating sources..."), QString(), 0, 0, this);
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setCancelButton(nullptr);
  progress.setMinimumDuration(0); // show immediately
  progress.show();

  // process events so dialog is painted before heavy work
  QApplication::processEvents();

  this->texture_manager.update();
  this->texture_manager.save();
  this->update_table_rows();

  progress.close();
}

void TextureDownloader::update_table_rows()
{
  Logger::log()->trace("TextureDownloader::update_table_rows");

  this->table_model->removeRows(0, this->table_model->rowCount());

  // if the database is empty write an hint on how to update sources
  if (this->texture_manager.is_empty())
  {
    this->table_model->appendRow(
        {new QStandardItem(),
         new QStandardItem(
             "Use the menu bar 'Texture sources' to populate the texture database.")});
    // TODO hardcoded
    this->table_view->setSpan(0, 1, 1, 7);
    return;
  }

  // else fill-in table
  this->table_view->setSpan(0, 0, 1, 1);

  for (auto &[id, tex] : this->texture_manager.get_textures())
  {
    QList<QStandardItem *> items;

    // thumbnail
    QStandardItem *img_item = new QStandardItem;
    QImage         img = QImage(this->texture_manager.get_thumbnail_path(id).c_str());

    if (!img.isNull())
    {
      QPixmap pix = QPixmap::fromImage(img);
      img_item->setData(pix, Qt::DecorationRole);
    }

    items.append(img_item);

    {
      QStandardItem *check_item = new QStandardItem;
      check_item->setCheckable(true);
      check_item->setCheckState(tex.get_is_pinned() ? Qt::Checked : Qt::Unchecked);
      check_item->setText(tex.get_is_pinned() ? "Y" : "N");
      items.append(check_item);
    }

    items.append(new QStandardItem(id.c_str()));
    items.append(new QStandardItem(tex.get_name().c_str()));
    items.append(new QStandardItem(tex.get_source().c_str()));

    // add textures
    for (auto &r : all_texture_types)
    {
      if (tex.has_texture(r, this->res))
      {
        QStandardItem *check_item = new QStandardItem;
        check_item->setCheckable(true);
        check_item->setCheckState(Qt::Unchecked);
        items.append(check_item);
      }
      else
      {
        items.append(new QStandardItem("Not available"));
      }
    }

    this->table_model->appendRow(items);
  }

  if (this->first_table_view_creation)
  {
    this->table_view->sortByColumn(1, Qt::DescendingOrder);
    this->first_table_view_creation = false;
  }
}

} // namespace qtd
