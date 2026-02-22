/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <memory>
#include <string>

#include <QImage>

#include "nlohmann/json.hpp"

namespace qtd
{

// --------------------------
// enums...
// --------------------------

enum TextureRes : int
{
  R1K,
  R2K,
  R4K,
  R8K,
  RUNKNOWN
};

static std::vector<TextureRes> all_texture_res = {TextureRes::R1K,
                                                  TextureRes::R2K,
                                                  TextureRes::R4K,
                                                  TextureRes::R8K};

static std::map<TextureRes, std::string> texture_res_as_string = {
    {TextureRes::R1K, "1k"},
    {TextureRes::R2K, "2k"},
    {TextureRes::R4K, "4k"},
    {TextureRes::R8K, "8k"},
    {TextureRes::RUNKNOWN, "unknown"},
};

enum TextureType : int
{
  DIFFUSE,
  NORMAL,
  DISPLACEMENT
};

static std::vector<TextureType> all_texture_types = {TextureType::DIFFUSE,
                                                     TextureType::NORMAL,
                                                     TextureType::DISPLACEMENT};

static std::map<TextureType, std::string> texture_type_as_string = {
    {TextureType::DIFFUSE, "Diffuse"},
    {TextureType::NORMAL, "Normal"},
    {TextureType::DISPLACEMENT, "Displacement"},
};

// --------------------------
// Texture
// --------------------------

class Texture
{
public:
  Texture() = default;

  void           json_from(nlohmann::json const &json);
  nlohmann::json json_to() const;

  std::string             get_id() const;
  bool                    get_is_pinned() const;
  std::string             get_name() const;
  std::string             get_source() const;
  std::vector<TextureRes> get_texture_resolutions(const TextureType &texture_type) const;
  std::string             get_texture_url(const TextureType &texture_type,
                                          const TextureRes  &texture_res) const;
  bool                    has_texture(const TextureType &texture_type) const;
  std::string             get_thumbnail_url() const;
  bool has_texture(const TextureType &texture_type, const TextureRes &texture_res) const;
  void set_id(const std::string &new_id);
  void set_is_pinned(bool new_state);

  bool from_poly_haven(const std::string    &asset_id,
                       const nlohmann::json &json_asset_list);

private:
  // --- Members
  std::string id; // unique ID

  std::string              name;
  std::string              source; // download origin
  std::string              id_from_source;
  std::string              thumbnail_url;
  std::vector<std::string> tags;
  bool                     is_pinned = false;

  std::map<std::string, std::string> diffuse_urls;
  std::map<std::string, std::string> normal_urls;
  std::map<std::string, std::string> displacement_urls;
};

} // namespace qtd