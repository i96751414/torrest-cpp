#ifndef TORREST_TORRENT_H
#define TORREST_TORRENT_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "boost/optional.hpp"
#include "boost/shared_array.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "spdlog/spdlog.h"

#include "enums.h"
#include "fwd.h"

namespace torrest { namespace bittorrent {

#if !TORREST_LEGACY_READ_PIECE

    struct PieceData {
        int size;
        boost::shared_array<char> buffer;
        std::chrono::steady_clock::time_point read_at;
    };

#endif //TORREST_LEGACY_READ_PIECE

    struct TorrentInfo {
        std::string info_hash;
        std::string name;
        std::int64_t size;
    };

    struct TorrentStatus {
        std::int64_t total;
        std::int64_t total_done;
        std::int64_t total_wanted;
        std::int64_t total_wanted_done;
        double progress;
        int download_rate;
        int upload_rate;
        bool paused;
        bool has_metadata;
        State state;
        int seeders;
        int seeders_total;
        int peers;
        int peers_total;
        std::int64_t seeding_time;
        std::int64_t finished_time;
        std::int64_t active_time;
        std::int64_t all_time_download;
        std::int64_t all_time_upload;
    };

    class Torrent : public std::enable_shared_from_this<Torrent> {
        friend class Service;

        friend class File;

        friend class Reader;

    public:
        Torrent(std::shared_ptr<ServiceSettings> pSettings,
                libtorrent::torrent_handle pHandle,
                std::string pInfoHash,
                std::shared_ptr<spdlog::logger> pLogger);

        void pause();

        void resume();

        void set_priority(libtorrent::download_priority_t pPriority);

        void check_available_space(const std::string &pPath);

        void check_save_resume_data() const;

        TorrentInfo get_info() const;

        TorrentStatus get_status() const;

        State get_state() const;

        std::vector<std::shared_ptr<File>> get_files() const;

        std::shared_ptr<File> get_file(int pIndex) const;

        const std::string &get_info_hash() const {
            return mInfoHash;
        }

    private:
        void handle_metadata_received();

#if !TORREST_LEGACY_READ_PIECE

        void store_piece(libtorrent::piece_index_t pPiece, int pSize, const boost::shared_array<char> &pBuffer);

        void cleanup_pieces(const std::chrono::milliseconds &pExpiration);

        void schedule_read_piece(libtorrent::piece_index_t pPiece);

        PieceData read_scheduled_piece(libtorrent::piece_index_t pPiece,
                                       const boost::optional<std::chrono::time_point<std::chrono::steady_clock>> &pWaitUntil);

        PieceData read_piece(libtorrent::piece_index_t pPiece,
                             const boost::optional<std::chrono::time_point<std::chrono::steady_clock>> &pWaitUntil);

        std::unordered_map<libtorrent::piece_index_t, PieceData> mPieces;

#endif //TORREST_LEGACY_READ_PIECE

        void wait_for_piece(libtorrent::piece_index_t pPiece,
                            const boost::optional<std::chrono::time_point<std::chrono::steady_clock>> &pUntil) const;

        State get_torrent_state() const;

        std::int64_t get_bytes_missing(const std::vector<libtorrent::piece_index_t> &pPieces) const;

        bool verify_buffering_state() const;

        std::shared_ptr<spdlog::logger> mLogger;
        libtorrent::torrent_handle mHandle;
        std::shared_ptr<ServiceSettings> mSettings;
        std::string mInfoHash;
        std::string mDefaultName;
        std::vector<std::shared_ptr<File>> mFiles;
        mutable std::mutex mMutex;
        mutable std::mutex mFilesMutex;
        mutable std::mutex mPiecesMutex;
        mutable std::condition_variable mPiecesCv;
        std::atomic<bool> mPaused{};
        std::atomic<bool> mHasMetadata;
        std::atomic<bool> mClosed;
    };

}}

#endif //TORREST_TORRENT_H
