/*
 * PS5DreamcastX USB game library discovery.
 * Included by shell/ps5/ps5_main.cpp; all standard headers are provided there.
 * The drive root is never recursively searched: only named folders.
 *
 * Supported:
 * /mnt/usb0/PS5DreamcastX/<ROM>
 * /mnt/usb0/PS5DreamcastX/games/<ROM> (preferred when present)
 * /mnt/usb1/... through /mnt/usb7/...
 * The established Flycast/naomi/arcade USB folders remain supported.
 */
static std::vector<std::string> discoverDreamcastXUsbDirs(const std::string& mountPrefix, int drives)
{
    std::vector<std::string> paths;
    for (int i = 0; i < drives; ++i)
    {
        const std::string mount = mountPrefix + std::to_string(i);
        DIR *dir = opendir(mount.c_str());
        if (!dir) continue;
        while (const dirent *entry = readdir(dir))
        {
            if (entry->d_name[0] == '.') continue;
            std::string folder = entry->d_name;
            std::string low = folder;
            for (char& c : low) c = (char)tolower((unsigned char)c);
            bool accepted = low == "ps5dreamcastx" || low == "flycast" || low == "dreamcast"
                || low == "dc" || low == "naomi" || low == "atomiswave" || low == "arcade";
            if (!accepted) continue;
            const std::string base = mount + "/" + folder;
            struct stat st{};
            // USB directory entries can report DT_UNKNOWN. lstat works reliably.
            if (lstat(base.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) continue;
            const std::string gameDir = base + "/games";
            if (low == "ps5dreamcastx" && lstat(gameDir.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                paths.push_back(gameDir);
            else
                paths.push_back(base);
        }
        closedir(dir);
    }
    return paths;
}
