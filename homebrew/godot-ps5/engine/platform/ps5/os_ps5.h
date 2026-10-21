// SPDX-License-Identifier: MIT
#pragma once
#include "drivers/unix/os_unix.h"

class OS_PS5 final : public OS_Unix {
    MainLoop *main_loop = nullptr;
protected:
    void initialize() override;
    void finalize() override;
    void delete_main_loop() override;
    void set_main_loop(MainLoop *loop) override;
public:
    String get_identifier() const override;
    String get_name() const override;
    String get_config_path() const override;
    String get_data_path() const override;
    String get_cache_path() const override;
    String get_user_data_dir(const String &user) const override;
    MainLoop *get_main_loop() const override;
    void run();
};
