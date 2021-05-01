#ifndef TORREST_EXCEPTIONS_H
#define TORREST_EXCEPTIONS_H

namespace torrest {
    class BittorrentException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class InvalidServiceException : public BittorrentException {
        using BittorrentException::BittorrentException;
    };

    class InvalidInfoHashException : public BittorrentException {
        using BittorrentException::BittorrentException;
    };

    class InvalidTorrentException : public BittorrentException {
        using BittorrentException::BittorrentException;
    };

    class DuplicateTorrentException : public BittorrentException {
    public:
        DuplicateTorrentException(const char *pMessage, std::string pInfoHash)
                : BittorrentException(pMessage),
                  mInfoHash(std::move(pInfoHash)) {}

        const std::string &get_info_hash() const {
            return mInfoHash;
        }

    private:
        std::string mInfoHash;
    };

    class LoadTorrentException : public BittorrentException {
        using BittorrentException::BittorrentException;
    };
}

#endif //TORREST_EXCEPTIONS_H
