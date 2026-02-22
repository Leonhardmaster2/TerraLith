/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <QObject>

#include "qtd/texture.hpp"

namespace qtd
{

// --------------------------
// TextureKey
// --------------------------

// facilitate texture search by id/type/resolution
struct TextureKey
{
  std::string id;
  TextureType type;
  TextureRes  res;

  bool operator==(const TextureKey &other) const
  {
    return id == other.id && type == other.type && res == other.res;
  }

  std::string to_string() const
  {
    return this->id + "_" + texture_type_as_string.at(this->type) + "_" +
           texture_res_as_string.at(this->res);
  }
};

// --------------------------
// TextureManager
// --------------------------

class TextureManager
{
public:
  explicit TextureManager(const std::string &storage_path_ = "");

  std::string                     get_storage_path() const;
  std::map<std::string, Texture> &get_textures();
  std::string                     get_texture_path(const TextureKey &texture_key) const;
  std::string                     get_thumbnail_path(const std::string &tex_id) const;
  bool                            is_empty() const;
  void                            set_storage_path(const std::string &new_path);

  // does not override existing file (returns path to file)
  std::string try_download_texture(const TextureKey &texture_key,
                                   bool              force_download = false) const;

  void load();
  void save() const;

  void update();
  void update_from_poly_haven();

private:
  void           file_from(const std::string &fname);
  void           file_to(const std::string &fname) const;
  void           json_from(nlohmann::json const &json);
  nlohmann::json json_to() const;

  // --- Members
  std::string                    storage_path;
  std::map<std::string, Texture> textures;
};

} // namespace qtd