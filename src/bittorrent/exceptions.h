#ifndef TORREST_EXCEPTIONS_H
#define TORREST_EXCEPTIONS_H

namespace torrest { namespace bittorrent {

    class BittorrentException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class InvalidInfoHashException : public BittorrentException {
        using BittorrentException::BittorrentException;
    };

    class InvalidTorrentException : public BittorrentException {
        using BittorrentException::BittorrentException;
    };

    class NoMetadataException : public BittorrentException {
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

}}

#endif //TORREST_EXCEPTIONS_H
