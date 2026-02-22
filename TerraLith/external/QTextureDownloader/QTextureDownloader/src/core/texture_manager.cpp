/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <QApplication>
#include <QSettings>

#include <filesystem>

#include "qtd/config.hpp"
#include "qtd/image_fetcher.hpp"
#include "qtd/json_fetcher.hpp"
#include "qtd/logger.hpp"
#include "qtd/texture_manager.hpp"
#include "qtd/utils.hpp"

namespace qtd
{

TextureManager::TextureManager(const std::string &storage_path_)
{
  Logger::log()->trace("TextureManager::TextureManager");

  // define storage path
  if (storage_path_.empty())
  {
    // not explicitely provided => use the one from the saved
    // settings or default one as a fallback
    QSettings   settings("olink", "QTextureDownloader");
    std::string path = settings.value("storage_path", "").toString().toStdString();
    if (path.empty())
      this->storage_path = "texture_downloader";
    else
      this->storage_path = path;
  }
  else
  {
    this->storage_path = storage_path_;
  }

  // create storage
  std::filesystem::path dir = std::filesystem::path(this->storage_path);
  if (!std::filesystem::exists(dir))
  {
    Logger::log()->info("TextureManager::TextureManager: creating storage repertory {}",
                        dir.string());
    std::filesystem::create_directories(dir);
  }
}

bool TextureManager::is_empty() const { return this->textures.size() == 0; }

void TextureManager::file_from(const std::string &fname)
{
  nlohmann::json json = json_from_file(fname);
  this->json_from(json);
}

void TextureManager::file_to(const std::string &fname) const
{
  json_to_file(this->json_to(), fname);
}

std::string TextureManager::get_storage_path() const { return this->storage_path; }

std::map<std::string, Texture> &TextureManager::get_textures() { return this->textures; }

std::string TextureManager::get_texture_path(const TextureKey &texture_key) const
{
  return this->storage_path + "/" + texture_key.to_string() + ".png";
}

std::string TextureManager::get_thumbnail_path(const std::string &tex_id) const
{
  return this->storage_path + "/" + tex_id + "_thumbnail.png";
}

void TextureManager::json_from(nlohmann::json const &j)
{
  this->textures.clear();

  for (auto &[key, value] : j.items())
  {
    this->textures[key] = Texture();
    this->textures[key].json_from(value);
  }
}

nlohmann::json TextureManager::json_to() const
{
  nlohmann::json json;

  for (auto &[key, tex] : this->textures)
    json[key] = tex.json_to();

  return json;
}

void TextureManager::load() { this->file_from(this->storage_path + "/db.json"); }

void TextureManager::save() const
{
  QSettings settings("olink", "QTextureDownloader");
  settings.setValue("storage_path", this->storage_path.c_str());

  this->file_to(this->storage_path + "/db.json");
}

void TextureManager::set_storage_path(const std::string &new_path)
{
  this->storage_path = new_path;
  this->update();
}

std::string TextureManager::try_download_texture(const TextureKey &texture_key,
                                                 bool              force_download) const
{
  if (!textures.contains(texture_key.id))
    return "";

  Texture tex = this->textures.at(texture_key.id);

  if (!tex.has_texture(texture_key.type, texture_key.res))
    return "";

  std::string fname = this->get_texture_path(texture_key);

  // check if file exists and download it if not
  std::filesystem::path path = std::filesystem::path(fname);

  if (!std::filesystem::exists(path) || force_download)
  {
    std::string url = tex.get_texture_url(texture_key.type, texture_key.res);

    Logger::log()->trace("TextureManager::try_download_texture: downloading {}", url);
    bool ok = download_file(url, fname);

    if (!ok)
      return "";
  }

  return fname;
}

void TextureManager::update()
{
  Logger::log()->trace("TextureManager::update");
  this->update_from_poly_haven();
}

void TextureManager::update_from_poly_haven()
{
  Logger::log()->trace("TextureManager::update_from_poly_haven");

  // --- retrieve asset list

  std::vector<std::string> asset_ids;

  JsonFetcher    json_fetcher;
  nlohmann::json json_asset_list = json_fetcher.fetch_sync(
      "https://api.polyhaven.com/assets?type=textures");

  if (json_asset_list.empty())
  {
    Logger::log()->error(
        "TextureManager::update_from_poly_haven: could not fetch asset list");
    return;
  }

  for (auto &e : json_asset_list.items())
    asset_ids.push_back(e.key());

  // --- update textures data

  int k = 0; // TODO DBG

  for (auto &source_id : asset_ids)
  {
    // build up a unique ID based on the source and the source ID
    const std::string id = "PolyHaven_" + source_id;

    Logger::log()->info("TextureManager::update_from_poly_haven: texture {}", id);

    // create and/or replace
    Texture new_texture = Texture();
    new_texture.set_id(id);
    bool ok = new_texture.from_poly_haven(source_id, json_asset_list);

    if (ok)
      this->textures[id] = new_texture;
    else
      continue;

    // download thumbnail
    std::string url = this->textures.at(id).get_thumbnail_url();
    std::string fname = this->get_thumbnail_path(id);

    Logger::log()->trace(
        "TextureManager::update_from_poly_haven: downloading thumbnail {}",
        url);
    ok = download_file(url, fname);

    // TODO TEST
    // k++;
    // if (k > 8)
    //   break;
  }
}

} // namespace qtd
