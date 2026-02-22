/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include <fstream>

#include <QBuffer>
#include <QByteArray>

#include "qtd/logger.hpp"
#include "qtd/utils.hpp"

namespace qtd
{

nlohmann::json json_from_file(const std::string &fname)
{
  nlohmann::json json;
  std::ifstream  file(fname);

  if (file.is_open())
  {
    file >> json;
    file.close();
    Logger::log()->trace("json_from_file: JSON successfully loaded from {}", fname);
  }
  else
  {
    Logger::log()->error("json_from_file: Could not open file {} to load JSON", fname);
  }

  return json;
}

void json_to_file(const nlohmann::json &json,
                  const std::string    &fname,
                  bool                  merge_with_existing_content)
{
  nlohmann::json final_json = json;

  if (merge_with_existing_content)
  {
    std::ifstream infile(fname);
    if (infile.is_open())
    {
      try
      {
        nlohmann::json existing;
        infile >> existing;
        infile.close();

        // Merge new JSON into existing JSON
        existing.merge_patch(json);
        final_json = existing;

        Logger::log()->trace("json_to_file: merged JSON with existing content in {}",
                             fname);
      }
      catch (const std::exception &e)
      {
        Logger::log()->warn(
            "json_to_file: Could not parse existing JSON in {} ({}). Overwriting "
            "instead.",
            fname,
            e.what());
      }
    }
  }

  std::ofstream outfile(fname);
  if (outfile.is_open())
  {
    outfile << final_json.dump(4);
    outfile.close();
    Logger::log()->trace("json_to_file: JSON successfully written to {}", fname);
  }
  else
  {
    Logger::log()->error("json_to_file: Could not open file {} to save JSON", fname);
  }
}

} // namespace qtd
