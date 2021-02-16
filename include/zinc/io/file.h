//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox                                                   //
//                                                                           //
// Licensed under the Apache License, Version 2.0 (the "License");           //
// you may not use this file except in compliance with the License.          //
// You may obtain a copy of the License at                                   //
//                                                                           //
//    http://www.apache.org/licenses/LICENSE-2.0                             //
//                                                                           //
// Unless required by applicable law or agreed to in writing, software       //
// distributed under the License is distributed on an "AS IS" BASIS,         //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  //
// See the License for the specific language governing permissions and       //
// limitations under the License.                                            //
//                                                                           //
//======---------------------------------------------------------------======//

#ifndef ZINC_IO_FILE
#define ZINC_IO_FILE

#include "zinc/io/concepts.h"
#include "zinc/types/aliases.h"
#include "zinc/types/concepts.h"
#include "zinc/types/strings.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>

namespace zinc
{
    namespace detail
    {
        struct CantOpenDirPlaceholder
        {};

        struct AccessDeniedPlaceholder
        {};

        struct FileUnreadablePlaceholder
        {};

        struct FileUnwritablePlaceholder
        {};
    } // namespace detail

    /// Thrown by `open_file_as` when the file being opened isn't actually a file
    class BadFileOpen : public std::logic_error
    {
    public:
        explicit BadFileOpen(detail::CantOpenDirPlaceholder, const std::filesystem::path& file)
            : logic_error("File '" + file.string() + "' is a directory, not a file!")
        {}

        explicit BadFileOpen(detail::AccessDeniedPlaceholder, const std::filesystem::path& file)
            : logic_error("Unable to open file '" + file.string() + "'!")
        {}
    };

    /// Thrown by file operations if the underlying file is not usable by the program in X way
    class BadFileOperation : public std::logic_error
    {
    public:
        explicit BadFileOperation(detail::FileUnreadablePlaceholder, const std::filesystem::path& file)
            : logic_error("File '" + file.string() + "' non-readable by the current program!")
        {}

        explicit BadFileOperation(detail::FileUnwritablePlaceholder, const std::filesystem::path& file)
            : logic_error("File '" + file.string() + "' is non-writable by the current program!")
        {}
    };

    /// RAII object that actually represents a **unique handle to a file**,
    /// instead of a file **stream**. Handles simple reads/writes with no B.S,
    /// meant for simple I/O with small files.
    ///
    /// Will read the entire file in at once, will not attempt to "save
    /// memory" or anything. As was mentioned, this type is not designed for
    /// specialized use cases; if you have those special use cases, you know
    /// how to do it.
    ///
    /// # Notes
    /// - `BasicFile` does keep track of times that it modified files, and
    ///   if that time changes without `BasicFile` having done it it will re-read the entire
    ///   file to update contents. Note that these checks **only** happen on write operations,
    ///   not any `const` operations.
    ///
    /// - If those times are "close enough" the filesystem may report it at the same time that
    ///   the class has. Use .notify_changed() to force an update.
    ///
    /// - If a file is non-readable, `content` will be empty, and writes will not try to keep it in sync.
    ///
    /// - If a file is non-writable, all write operations will throw.
    template <Charlike CharT> class BasicFile
    {
    public:
        using pos_type = PosT<std::basic_fstream<CharT>>;
        using traits_type = std::char_traits<CharT>;
        using value_type = CharT;
        using allocator_type = std::allocator<CharT>;
        using size_type = typename std::allocator_traits<allocator_type>::size_type;
        using difference_type = typename std::allocator_traits<allocator_type>::difference_type;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = typename std::allocator_traits<allocator_type>::pointer;
        using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /// However much I may feel like this shouldn't be usable, it's needed for
        /// use in containers.
        BasicFile() noexcept = default;

        /// Opens a `BasicFile` to a particular path. If the file being opened
        /// is a directory or cannot be read with an `fstream`, `BadFileOpen` is thrown.
        ///
        /// # Parameters
        /// - `path`: The path to try to open
        /// - `mode`: An openmode to open the file with. Note, the real file is opened with
        ///           `std::ios_base::in` and `std::ios_base::out` as well.
        BasicFile(std::filesystem::path path, std::ios_base::openmode mode)
            : file_(std::make_unique<std::basic_fstream<CharT>>(path, mode | std::ios_base::in | std::ios_base::out))
            , path_(std::filesystem::absolute(path))
        {
            namespace fs = std::filesystem;

            if (fs::is_directory(path_)) throw BadFileOpen(detail::CantOpenDirPlaceholder{}, path_);

            if (!fs::exists(path_))
            {
                std::fstream f(path_, std::ios_base::app);
                file_->clear();
                file_->open(path_, mode | std::ios_base::in | std::ios_base::out);
            }

            if (!file_->is_open())
            {
                // figures out r/w regardless of user-given mode
                readwrite_ = can_read_or_write();

                if (readwrite_ == std::pair{false, false})
                {
                    throw BadFileOpen(detail::AccessDeniedPlaceholder{}, path_);
                }
            }

            update_contents();
        }

        /// Simply copy-constructs all members, effectively creates a new
        /// `BasicFile` going to the same path
        BasicFile(const BasicFile&) = default;

        /// Simply move-constructs all members, makes the old `BasicFile`
        /// empty
        BasicFile(BasicFile&&) noexcept = default;

        /// Closes the file stream and de-allocates any memory the type holds
        ~BasicFile() = default;

        /// Assigns the `BasicFile` to a new file, properly closing/clearing any
        /// old containers it contains
        BasicFile& operator=(const BasicFile&) = default;

        /// Assigns the `BasicFile` to a new file, properly closing/clearing any
        /// old containers it contains and emptying the other file
        BasicFile& operator=(BasicFile&&) noexcept = default;

        /// Opens a file after the construction of a file. This is **not** how the type
        /// is meant to be used, it exists solely because the default constructor exists
        /// and there needs to be a reason to use the type after being default-constructed.
        ///
        /// Equivalent to:
        /// ```cpp
        /// file = BasicFile(std::move(path), mode);
        /// ```
        ///
        /// # Parameters
        /// - `path`: The path to open
        /// - `mode`: The mode to give to the internal `fstream`.
        void open(std::filesystem::path path, std::ios_base::openmode mode)
        {
            // im so sorry
            *this = BasicFile(std::move(path), mode);
        }

        /// Truncates the file and **then** writes `object` to the file, and
        /// updates the internal file content buffer. Flushes the file. Invalidates
        /// previous iterators/`string_view`s/pointers.
        ///
        /// # Parameters
        /// - `object`: The object to print. Requires a valid `<<`
        ///             overload for `std::ostream`
        ///
        /// # Returns
        /// Returns `*this` to enable chaining
        template <Printable T> BasicFile& truncate(const T& object)
        {
            if (writable())
            {
                std::filesystem::resize_file(path_, 0);
                data_.clear();
                file_->seekg(0, std::ios::beg);
                *file_ << object;

                update_contents();

                last_updated_ = std::filesystem::last_write_time(path());

                return *this;
            }

            throw BadFileOperation(detail::FileUnwritablePlaceholder{}, path());
        }

        /// Writes `object` to the file and updates the internal file content buffer.
        /// Flushes the file, not good for use in a hot loop. Invalidates any
        /// previous iterators/`string_view`s/pointers.
        ///
        /// # Parameters
        /// - `object`: The object to print. Requires a valid `<<`
        ///             overload for `std::ostream`
        ///
        /// # Returns
        /// Returns `*this` to enable chaining
        template <Printable T> BasicFile& overwrite(pos_type position, const T& object)
        {
            update_contents();

            if (writable())
            {
                const auto size = real_file_length();
                file_->seekg(position);
                const auto new_size = write_and_report(object);
                const auto difference = new_size - size;

                [[likely]] if (readable())
                {
                    file_->seekg(position);
                    data_.resize(data_.size() + difference);

                    // overwrite could in theory just be doing an append, or doing partially an overwrite
                    // and partially doing an append
                    file_->read(data_.data() + position, data_.size() - position + difference);
                }

                last_updated_ = std::filesystem::last_write_time(path());

                return *this;
            }

            throw BadFileOperation(detail::FileUnwritablePlaceholder{}, path());
        }

        /// Appends `object` to the file and updates the internal file content buffer.
        /// Flushes the file, not good for use in a hot loop. Invalidates any
        /// previous iterators/`string_view`s/pointers.
        ///
        /// # Parameters
        /// - `object`: The object to print. Requires a valid `<<` overload for `std::ostream`
        ///
        /// # Returns
        /// Returns `*this` to enable chaining
        template <Printable T> BasicFile& append(const T& object)
        {
            update_contents();

            if (writable())
            {
                // read_file_length seeks file_ to the end
                const auto size = real_file_length();
                const auto new_size = write_and_report(object);
                const auto difference = new_size - size;

                [[likely]] if (readable())
                {
                    data_.resize(data_.size() + difference);
                    file_->seekg(-difference, std::ios_base::end);
                    file_->read(data_.data() + data_.size() - difference, difference);
                }

                last_updated_ = std::filesystem::last_write_time(path());

                return *this;
            }

            throw BadFileOperation(detail::FileUnwritablePlaceholder{}, path());
        }

        /// Gets the file's length. Like `std::string`, it pays no mind to encoding.
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }

        /// Gets the file's content as a `string_view`. Valid until any modifying operations are done
        /// to the file, in which case a new one can be obtained from `content` again.
        ///
        /// Note: modifying operations done to the underlying file outside of this `BasicFile`
        /// instance do not technically invalidate the content string, although the contents
        /// will be out of sync until the file is notified.
        [[nodiscard]] std::basic_string_view<CharT> content() const& noexcept { return data_; }

        /// Moves the file's content into a `string`
        ///
        /// Note: works the same way as the other `content` overload, it will
        /// **try** to keep track of file changes.
        [[nodiscard]] std::basic_string<CharT> content() && noexcept { return std::move(data_); }

        /// Forces a re-read of the file to update the internal file cache
        ///
        /// Use this as a last resort if you're having issues with containers being
        /// written by another source and `BasicFile` not knowing, but come on.
        /// Is there really not another way of going about this?
        void notify_changed()
        {
            // dirty hack but it works. If this is being called, timings are likely
            // so tight that it wouldn't know anyway
            last_updated_ = std::filesystem::file_time_type::min();

            update_contents();
        }

        /// Returns if the file is actually readable by the program.
        [[nodiscard]] bool readable() const noexcept { return readwrite_.first; }

        /// Returns if the file is actually writable by the program.
        [[nodiscard]] bool writable() const noexcept { return readwrite_.second; }

        /// Returns the path of the file. `BasicFile` uses absolute paths exclusively, so
        /// the path returned is absolute.
        [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

        /// Returns the value at a specific index in the file
        [[nodiscard]] value_type operator[](size_type index) const noexcept { return data_[index]; }

        /// Returns the value at a specific index in the file
        [[nodiscard]] value_type at(size_type index) const
        {
            if (data_.size() >= index)
            {
                throw std::out_of_range{"BasicFile::at: index " + std::to_string(index) + " out of range, len is "
                                        + std::to_string(data_.size())};
            }

            return data_[index];
        }

        /// Checks if the file has a length of 0
        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

        /// Returns the first character of the file
        [[nodiscard]] value_type front() const noexcept { return data_.front(); }

        /// Returns the last character of the file
        [[nodiscard]] value_type back() const noexcept { return data_.back(); }

        /// Returns a pointer to the first character in the file's content
        [[nodiscard]] const_pointer raw() const noexcept { return data_.data(); }

        [[nodiscard]] operator std::basic_string_view<CharT>() const noexcept { return content(); }

        [[nodiscard]] const_iterator begin() const noexcept { return data_.begin(); }

        [[nodiscard]] const_iterator cbegin() const noexcept { return data_.begin(); }

        [[nodiscard]] const_iterator end() const noexcept { return data_.end(); }

        [[nodiscard]] const_iterator cend() const noexcept { return data_.cend(); }

        [[nodiscard]] const_iterator rbegin() const noexcept { return data_.rbegin(); }

        [[nodiscard]] const_iterator rcbegin() const noexcept { return data_.rbegin(); }

        [[nodiscard]] const_iterator rend() const noexcept { return data_.rend(); }

        [[nodiscard]] const_iterator rcend() const noexcept { return data_.rcend(); }

    protected:
        // writes an object, flushes the file, and reports the new size.
        // does not restore streams original position, it's left at the end
        template <Printable T> [[nodiscard]] pos_type write_and_report(const T& object)
        {
            *file_ << object << std::flush;

            return real_file_length();
        }

        [[nodiscard]] pos_type real_file_length() const noexcept
        {
            file_->seekg(0, std::ios_base::end);

            return file_->tellg();
        }

        // tries opening the file in IN mode and OUT mode to see what it's able to do
        // return value is {read, write}
        std::pair<bool, bool> can_read_or_write()
        {
            std::pair<bool, bool> readwrite = {true, true};

            file_->clear();
            file_->open(path_, std::ios_base::in);

            if (!file_->is_open()) readwrite.first = false;

            file_->clear();
            file_->open(path_, std::ios_base::out);

            if (!file_->is_open()) readwrite.second = false;

            return readwrite;
        }

        // updates the file contents if the file is newer than we're keeping track of
        void update_contents()
        {
            if (!readable()) throw BadFileOperation(detail::FileUnreadablePlaceholder{}, path_);

            if (std::filesystem::last_write_time(path_) != last_updated_)
            {
                data_.clear();

                [[likely]] if (const auto length = real_file_length(); length != 0)
                {
                    data_.resize(length);
                    file_->seekg(0, std::ios_base::beg);
                    file_->read(data_.data(), length);
                }

                last_updated_ = std::filesystem::last_write_time(path_);
            }
        }

    private:
        std::chrono::time_point<std::chrono::file_clock> last_updated_ = std::filesystem::file_time_type::min();
        std::basic_string<CharT> data_;
        std::unique_ptr<std::basic_fstream<CharT>> file_; // fstreams are big, and RAII objects get moved a lot
        std::filesystem::path path_;
        std::pair<bool, bool> readwrite_ = {true, true};
    };

    /// `BasicFile`s represent actual files, therefore there's no reason to compare anything except the paths.
    /// `BasicFile` paths are absolute, so no need to even do conversions there. Also, it doesn't really make sense
    /// to compare two files opened with different `CharT`s, and if it's really needed, comparison is possible
    /// with member functions.
    ///
    /// # Parameters
    /// - `lhs`: First file to compare
    /// - `rhs`: Second file to compare
    ///
    /// # Returns
    /// Whether both files are going to the same file
    template <Charlike CharT> bool operator==(const BasicFile<CharT>& lhs, const BasicFile<CharT>& rhs)
    {
        return lhs.path() == rhs.path();
    }

    /// Setting for if files should be opened in binary mode or not
    ///
    /// C++ file streams have a "binary" mode that prevents implicit conversion
    /// of `'\n'` to the platform's newline (whether it be CR, LF, or CRLF)
    enum class ConvertNewlines
    {
        enabled,
        disabled
    };

    /// Creates a `BasicFile<CharT>` from the input path and mode
    ///
    /// # Parameters:
    /// - `path`: The path to open
    /// - `mode`: The mode to pass to `BasicFile`s constructor (and internal `std::basic_fstream`).
    template <Charlike CharT> BasicFile<CharT> open_file_with(std::filesystem::path path, ConvertNewlines mode)
    {
        auto options = std::ios_base::in | std::ios_base::out;

        if (mode == ConvertNewlines::enabled)
        {
            options |= std::ios_base::binary;
        }

        return BasicFile<CharT>(std::move(path), options);
    }

    /// Convenience alias to `BasicFile<char>` for the most common use cases.
    using File = BasicFile<char>;

    /// Convenience function mapping to `open_file_with<char>(std::move(path), ConvertNewlines::disabled)`.
    inline File open_file(std::filesystem::path path, ConvertNewlines mode = ConvertNewlines::disabled)
    {
        return open_file_with<char>(std::move(path), mode);
    }

    /// Convenience function for opening and immediately reading a file.
    inline std::string read_file(std::filesystem::path path, ConvertNewlines mode = ConvertNewlines::disabled)
    {
        auto file = open_file(std::move(path), mode);

        return std::move(file).content();
    }
} // namespace zinc

#endif