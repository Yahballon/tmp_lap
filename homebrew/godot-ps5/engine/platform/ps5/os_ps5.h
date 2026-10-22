// SPDX-License-Identifier: MIT
#pragma once
#include "drivers/unix/os_unix.h"

class OS_PS5 final : public OS_Unix {
    MainLoop *main_loop = nullptr;
protected:
    void initialize() override;
    void finalize() override;
    void initialize_joypads() override;
    bool _check_internal_feature_support(const String &feature) override;
    void delete_main_loop() override;
    void set_main_loop(MainLoop *loop) override;
public:
    // PS5 sandbox: avoid unsupported libc getcwd and FreeBSD sysctl.
    String get_cwd() const override;
    Error set_cwd(const String &path) override;
    String get_executable_path() const override;
    String get_identifier() const override;
    String get_name() const override;
    String get_config_path() const override;
    String get_data_path() const override;
    String get_cache_path() const override;
    String get_user_data_dir(const String &user) const override;
    MainLoop *get_main_loop() const override;
    void run();
};
