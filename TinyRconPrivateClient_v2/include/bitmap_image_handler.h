#pragma once

#include <filesystem>
#include <format>
#include <vector>
#include <string>
#include <unordered_map>
#include "tiny_rcon_utility_classes.h"
#include "tiny_rcon_utility_functions.h"

using std::string;

class bitmap_image_handler : public disabled_copy_operations
  , public disabled_move_operations
{
  string bitmap_images_folder_path_;
  std::unordered_map<std::string, HBITMAP> bitmap_images_;

public:
  bitmap_image_handler() = default;
  // DISABLE_COPY_SEMANTICS(bitmap_image_handler);
  // DISABLE_MOVE_SEMANTICS(bitmap_image_handler);

  ~bitmap_image_handler()
  {
    for (auto &&[bitmap_image_name, bitmap_image_handle] : bitmap_images_) {
      if (bitmap_image_handle) {
        DeleteObject(bitmap_image_handle);
        bitmap_image_handle = NULL;
      }
    }
  }

  [[maybe_unused]] bool set_bitmap_images_folder_path(std::string bitmap_images_folder_path)
  {
    if (!check_if_file_path_exists(bitmap_images_folder_path.c_str()))
      return false;
    bitmap_images_folder_path_ = std::move(bitmap_images_folder_path);
    return true;
  }

  HBITMAP get_bitmap_image(const std::string &bitmap_image_name, const bool is_load_if_not_in_cache = true)
  {
    if ((!bitmap_images_.contains(bitmap_image_name) || bitmap_images_.at(bitmap_image_name) == NULL) && is_load_if_not_in_cache) {
      // const std::string bitmap_file_path{ format("{}\\{}.bmp", bitmap_images_folder_path_, bitmap_image_name) };
      if (!load_bitmap_image(bitmap_image_name) || !bitmap_images_.contains(bitmap_image_name))
        return NULL;
      return bitmap_images_.at(bitmap_image_name);
    }

    return bitmap_images_.contains(bitmap_image_name) ? bitmap_images_.at(bitmap_image_name) : (bitmap_images_.contains("nomap") ? bitmap_images_.at("nomap") : NULL);
  }

  const std::unordered_map<std::string, HBITMAP> get_bitmap_images() const noexcept
  {
    return bitmap_images_;
  }

  bool load_bitmap_images()
  {
    // const string bitmap_images_folder_path{ format("{}data\\images\\maps", main_app_->get_current_working_directory()) };
    if (!check_if_file_path_exists(bitmap_images_folder_path_.c_str())) {
      create_necessary_folders_and_files({ bitmap_images_folder_path_ });
      return false;
    }

    const string bmp_image_file_path_pattern{ format("{}\\*.bmp", bitmap_images_folder_path_) };

    WIN32_FIND_DATA file_data{};
    HANDLE read_file_data_handle{ FindFirstFile(bmp_image_file_path_pattern.c_str(), &file_data) };

    if (read_file_data_handle != INVALID_HANDLE_VALUE) {
      do {
        std::string image_name{ stl::helper::to_lower_case(file_data.cFileName) };
        if (image_name.ends_with(".bmp")) {
          image_name.erase(image_name.length() - 4, 4);

          if (bitmap_images_.contains(image_name) && bitmap_images_.contains(image_name) != NULL) {
            DeleteObject(bitmap_images_.at(image_name));
          }
          const string bmp_image_file_path{ format("{}\\{}", bitmap_images_folder_path_, file_data.cFileName) };
          bitmap_images_[image_name] = (HBITMAP)LoadImageA(NULL, bmp_image_file_path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        }

        ZeroMemory(&file_data, sizeof(WIN32_FIND_DATA));

      } while (FindNextFile(read_file_data_handle, &file_data));

      FindClose(read_file_data_handle);
      return true;
    }

    return false;
  }

  bool load_bitmap_image(const std::string &image_name)
  {
    // const string bitmap_images_folder_path{ format("{}data\\images\\maps", main_app_->get_current_working_directory()) };
    if (!check_if_file_path_exists(bitmap_images_folder_path_.c_str())) {
      create_necessary_folders_and_files({ bitmap_images_folder_path_ });
    }

    const std::string bitmap_file_path{ format("{}\\{}.bmp", bitmap_images_folder_path_, image_name) };
    if (!check_if_file_path_exists(bitmap_file_path.c_str())) {
      if (!download_bitmap_image_file(image_name.c_str(), bitmap_file_path.c_str()) || !check_if_file_path_exists(bitmap_file_path.c_str()))
        return false;
    }

    if (!bitmap_images_.contains(image_name) || bitmap_images_.at(image_name) == NULL) {
      bitmap_images_[image_name] = (HBITMAP)LoadImageA(NULL, bitmap_file_path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    }

    return bitmap_images_.contains(image_name) && bitmap_images_.at(image_name) != NULL;
  }
};
