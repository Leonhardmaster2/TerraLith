/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <string>

#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QWidget>

#include "nlohmann/json.hpp"

#include "qtd/texture_manager.hpp"

namespace qtd
{

class TextureDownloader : public QWidget
{
  Q_OBJECT

public:
  explicit TextureDownloader(const std::string &_title = "", QWidget *parent = nullptr);
  ~TextureDownloader();

  void set_texture_res(const TextureRes &new_res);

  // --- QWidget interface
  QSize sizeHint() const override;

signals:
  void textures_retrieved(const std::vector<std::string> &texture_paths);
  void window_closed();

public slots:
  void choose_storage_path();
  void purge_database();
  void retrieve_selected_textures();
  void unchecked_all_items();
  void update_sources();

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  void setup_connections();
  void setup_layout();
  void setup_menu_bar();
  void update_table_rows();

  // --- Members
  std::string    title;
  TextureManager texture_manager;
  TextureRes     res = TextureRes::R1K;
  bool           first_table_view_creation = true;

  QPushButton        *button_get_selected;
  QPushButton        *button_uncheck_items;
  QComboBox          *combo_res;
  QStandardItemModel *table_model;
  QTableView         *table_view;
};

} // namespace qtd