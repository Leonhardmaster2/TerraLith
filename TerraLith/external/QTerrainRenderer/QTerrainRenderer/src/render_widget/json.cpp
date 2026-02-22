/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/logger.hpp"
#include "qtr/render_widget.hpp"
#include "qtr/utils.hpp"

namespace qtr
{

void RenderWidget::json_from(nlohmann::json const &json)
{
  qtr::Logger::log()->trace("RenderWidget::json_from");

  // geometry
  if (json.contains("x") && json.contains("y") && json.contains("width") &&
      json.contains("height"))
  {
    int x = json["x"];
    int y = json["y"];
    int w = json["width"];
    int h = json["height"];

    this->setGeometry(x, y, w, h);
  }
  else
  {
    qtr::Logger::log()->error(
        "RenderWidget::json_from: could not parse the widget geometry data");
  }

  this->camera.json_from(json["camera"]);
  this->light.json_from(json["light"]);

  // data
  json_safe_get(json, "title", title);
  json_safe_get(json, "render_type", render_type);

  // GUI state
  json_safe_get(json, "wireframe_mode", wireframe_mode);
  json_safe_get(json, "auto_rotate_light", auto_rotate_light);
  json_safe_get(json, "auto_rotate_camera", auto_rotate_camera);

  // Camera parameters
  json_safe_get(json, "target", target);
  json_safe_get(json, "pan_offset", pan_offset);
  json_safe_get(json, "distance", distance);
  json_safe_get(json, "alpha_x", alpha_x);
  json_safe_get(json, "alpha_y", alpha_y);
  json_safe_get(json, "light_phi", light_phi);
  json_safe_get(json, "light_theta", light_theta);
  json_safe_get(json, "light_distance", light_distance);

  // Heightmap
  json_safe_get(json, "scale_h", scale_h);
  json_safe_get(json, "hmap_h0", hmap_h0);
  json_safe_get(json, "hmap_w", hmap_w);
  json_safe_get(json, "hmap_h", hmap_h);
  json_safe_get(json, "mesh_downsample_level", mesh_downsample_level);

  // Scene visibility
  json_safe_get(json, "render_plane", render_plane);
  json_safe_get(json, "render_points", render_points);
  json_safe_get(json, "render_path", render_path);
  json_safe_get(json, "render_hmap", render_hmap);
  json_safe_get(json, "render_rocks", render_rocks);
  json_safe_get(json, "render_trees", render_trees);
  json_safe_get(json, "render_water", render_water);

  // Normals
  json_safe_get(json, "normal_visualization", normal_visualization);
  json_safe_get(json, "normal_map_scaling", normal_map_scaling);

  // Gamma & tonemap
  json_safe_get(json, "gamma_correction", gamma_correction);
  json_safe_get(json, "apply_tonemap", apply_tonemap);

  // Shadows
  json_safe_get(json, "shadow_map_resolution", shadow_map_resolution);
  json_safe_get(json, "bypass_shadow_map", bypass_shadow_map);
  json_safe_get(json, "shadow_strength", shadow_strength);

  // Ambient occlusion
  json_safe_get(json, "add_ambiant_occlusion", add_ambiant_occlusion);
  json_safe_get(json, "ambiant_occlusion_strength", ambiant_occlusion_strength);
  json_safe_get(json, "ambiant_occlusion_radius", ambiant_occlusion_radius);

  // Textures
  json_safe_get(json, "bypass_texture_albedo", bypass_texture_albedo);

  // Water
  json_safe_get(json, "color_shallow_water", color_shallow_water);
  json_safe_get(json, "color_deep_water", color_deep_water);
  json_safe_get(json, "water_color_depth", water_color_depth);
  json_safe_get(json, "water_spec_strength", water_spec_strength);

  // Foam
  json_safe_get(json, "add_water_foam", add_water_foam);
  json_safe_get(json, "foam_color", foam_color);
  json_safe_get(json, "foam_depth", foam_depth);

  // Waves
  json_safe_get(json, "add_water_waves", add_water_waves);
  json_safe_get(json, "angle_spread_ratio", angle_spread_ratio);
  json_safe_get(json, "waves_alpha", waves_alpha);
  json_safe_get(json, "waves_kw", waves_kw);
  json_safe_get(json, "waves_amplitude", waves_amplitude);
  json_safe_get(json, "waves_normal_amplitude", waves_normal_amplitude);
  json_safe_get(json, "animate_waves", animate_waves);
  json_safe_get(json, "waves_speed", waves_speed);

  // Environment
  json_safe_get(json, "add_fog", add_fog);
  json_safe_get(json, "fog_color", fog_color);
  json_safe_get(json, "fog_density", fog_density);
  json_safe_get(json, "fog_height", fog_height);
  json_safe_get(json, "add_atmospheric_scattering", add_atmospheric_scattering);
  json_safe_get(json, "scattering_density", scattering_density);
  json_safe_get(json, "rayleigh_color", rayleigh_color);
  json_safe_get(json, "mie_color", mie_color);
  json_safe_get(json, "fog_strength", fog_strength);
  json_safe_get(json, "fog_scattering_ratio", fog_scattering_ratio);

  // Viewer 2D
  json_safe_get(json, "viewer2d_settings.zoom", viewer2d_settings.zoom);
  json_safe_get(json, "viewer2d_settings.offset", viewer2d_settings.offset);
  json_safe_get(json, "viewer2d_settings.hillshading", viewer2d_settings.hillshading);
  json_safe_get(json, "viewer2d_settings.sun_azimuth", viewer2d_settings.sun_azimuth);
  json_safe_get(json, "viewer2d_settings.sun_zenith", viewer2d_settings.sun_zenith);
  json_safe_get(json, "viewer2d_settings.cmap", viewer2d_settings.cmap);
}

nlohmann::json RenderWidget::json_to() const
{
  qtr::Logger::log()->trace("RenderWidget::json_to");

  nlohmann::json json;

  json = {
      // General
      {"title", title},
      {"render_type", render_type},

      // GUI state
      {"wireframe_mode", wireframe_mode},
      {"auto_rotate_light", auto_rotate_light},
      {"auto_rotate_camera", auto_rotate_camera},

      // Camera parameters
      {"target", target},
      {"pan_offset", pan_offset},
      {"distance", distance},
      {"alpha_x", alpha_x},
      {"alpha_y", alpha_y},
      {"light_phi", light_phi},
      {"light_theta", light_theta},
      {"light_distance", light_distance},

      // Heightmap
      {"scale_h", scale_h},
      {"hmap_h0", hmap_h0},
      {"hmap_w", hmap_w},
      {"hmap_h", hmap_h},
      {"mesh_downsample_level", mesh_downsample_level},

      // Scene visibility
      {"render_plane", render_plane},
      {"render_points", render_points},
      {"render_path", render_path},
      {"render_hmap", render_hmap},
      {"render_rocks", render_rocks},
      {"render_trees", render_trees},
      {"render_water", render_water},

      // Normals
      {"normal_visualization", normal_visualization},
      {"normal_map_scaling", normal_map_scaling},

      // Gamma & tonemap
      {"gamma_correction", gamma_correction},
      {"apply_tonemap", apply_tonemap},

      // Shadows
      {"shadow_map_resolution", shadow_map_resolution},
      {"bypass_shadow_map", bypass_shadow_map},
      {"shadow_strength", shadow_strength},

      // Ambient occlusion
      {"add_ambiant_occlusion", add_ambiant_occlusion},
      {"ambiant_occlusion_strength", ambiant_occlusion_strength},
      {"ambiant_occlusion_radius", ambiant_occlusion_radius},

      // Textures
      {"bypass_texture_albedo", bypass_texture_albedo},

      // Water
      {"color_shallow_water", color_shallow_water},
      {"color_deep_water", color_deep_water},
      {"water_color_depth", water_color_depth},
      {"water_spec_strength", water_spec_strength},

      // Foam
      {"add_water_foam", add_water_foam},
      {"foam_color", foam_color},
      {"foam_depth", foam_depth},

      // Waves
      {"add_water_waves", add_water_waves},
      {"angle_spread_ratio", angle_spread_ratio},
      {"waves_alpha", waves_alpha},
      {"waves_kw", waves_kw},
      {"waves_amplitude", waves_amplitude},
      {"waves_normal_amplitude", waves_normal_amplitude},
      {"animate_waves", animate_waves},
      {"waves_speed", waves_speed},

      // Environment
      {"add_fog", add_fog},
      {"fog_color", fog_color},
      {"fog_density", fog_density},
      {"fog_height", fog_height},
      {"add_atmospheric_scattering", add_atmospheric_scattering},
      {"scattering_density", scattering_density},
      {"rayleigh_color", rayleigh_color},
      {"mie_color", mie_color},
      {"fog_strength", fog_strength},
      {"fog_scattering_ratio", fog_scattering_ratio},

      // other classes
      {"camera", this->camera.json_to()},
      {"light", this->light.json_to()},

      // Viewer 2D
      {"viewer2d_settings.zoom", viewer2d_settings.zoom},
      {"viewer2d_settings.offset", viewer2d_settings.offset},
      {"viewer2d_settings.hillshading", viewer2d_settings.hillshading},
      {"viewer2d_settings.sun_azimuth", viewer2d_settings.sun_azimuth},
      {"viewer2d_settings.sun_zenith", viewer2d_settings.sun_zenith},
      {"viewer2d_settings.cmap", viewer2d_settings.cmap},
  };

  // geometry
  QRect geom = this->geometry();
  json["x"] = geom.x();
  json["y"] = geom.y();
  json["width"] = geom.width();
  json["height"] = geom.height();

  return json;
}

} // namespace qtr
