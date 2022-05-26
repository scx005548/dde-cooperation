#ifndef KEY_PAIR_H
#define KEY_PAIR_H

#include <filesystem>

class KeyPair {
public:
    enum class KeyType {
        ED25519,
    };

    explicit KeyPair(const std::filesystem::path &dataDir, KeyType type);

    bool load();

    std::string getFingerprint() const { return m_fingerprint; }

private:
    KeyType m_type;

    std::filesystem::path m_privatePath;
    std::filesystem::path m_publicPath;

    std::string m_fingerprint;

    bool generateNewKey();
};

#endif // !KEY_PAIR_H
