#pragma once
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

// Path helpers for code that has to keep working on character buffers: the plug-in API and the
// global directory strings hand out raw pointers, so they can't simply become std::filesystem::path.
// Where a buffer doesn't need to survive, use std::filesystem::path directly instead.
//
// Nothing here throws on a bad path. As with the buffers they replace, in-place functions assume
// the destination holds FilePath::MaxLength characters unless they are given a size.
namespace FilePath
{
	inline constexpr size_t MaxLength = 260;

	namespace detail
	{
		template <typename C>
		constexpr bool IsSeparator(C c) { return c == C('\\') || c == C('/'); }

		inline wint_t Fold(wchar_t c) { return std::towlower(c); }
		inline int Fold(char c) { return std::tolower(static_cast<unsigned char>(c)); }

		template <typename C>
		size_t Length(const C *s) { return std::char_traits<C>::length(s); }

		template <typename C>
		void CopyTo(C *dest, size_t dest_size, std::basic_string_view<C> source)
		{
			if (!dest_size)
				return;
			const size_t count = (std::min)(source.size(), dest_size - 1);
			std::char_traits<C>::move(dest, source.data(), count);
			dest[count] = C(0);
		}
	}

	// "scheme:" with a scheme of at least two characters, so "C:\file" is not a URL
	template <typename C>
	bool IsUrl(const C *path)
	{
		if (!path)
			return false;

		const C *p = path;
		while ((*p >= C('a') && *p <= C('z')) || (*p >= C('A') && *p <= C('Z')) || (*p >= C('0') && *p <= C('9')) || *p == C('+') || *p == C('-') || *p == C('.'))
			p++;
		return *p == C(':') && p - path >= 2;
	}

	// a URL whose scheme reaches over the network; local schemes such as cda://, linein:// and
	// file:// are not network URLs, and neither are schemes a plug-in invents for itself
	template <typename C>
	bool IsNetworkUrl(const C *path)
	{
		if (!IsUrl(path))
			return false;

		static constexpr const char *schemes[] = {
			"http", "https", "ftp", "ftps", "mms", "mmsh", "mmst", "mmsu",
			"rtsp", "rtspu", "rtmp", "rtmps", "rtp", "udp", "icy", "sc", "uvox", "unsv",
		};
		size_t length = 0;
		while (path[length] != C(':'))
			length++;
		for (const char *scheme : schemes)
		{
			size_t i = 0;
			while (i < length && scheme[i] && detail::Fold(path[i]) == detail::Fold(C(scheme[i])))
				i++;
			if (i == length && !scheme[i])
				return true;
		}
		return false;
	}

	template <typename C>
	bool IsRelative(const C *path)
	{
		return !path || !*path || !(*path == C('\\') || path[1] == C(':'));
	}

	template <typename C>
	bool IsUnc(const C *path)
	{
		if (!path || path[0] != C('\\') || path[1] != C('\\'))
			return false;
		if (path[2] != C('?'))
			return true;
		// long path forms are only UNC if they say so
		return path[3] == C('\\') && path[4] == C('U') && path[5] == C('N') && path[6] == C('C') && path[7] == C('\\');
	}

	// true when the path has no directory or drive part
	template <typename C>
	bool IsFileSpec(const C *path)
	{
		if (!path)
			return false;
		for (; *path; path++)
			if (*path == C('\\') || *path == C(':'))
				return false;
		return true;
	}

	// "\", "C:\", "\\server" or "\\server\share"
	template <typename C>
	bool IsRoot(const C *path)
	{
		if (!path || !*path)
			return false;
		if (*path == C('\\'))
		{
			if (!path[1])
				return true;
			if (path[1] != C('\\'))
				return false;
			bool seen_slash = false;
			for (const C *p = path + 2; *p; p++)
			{
				if (*p == C('\\'))
				{
					if (seen_slash || !p[1])
						return false;
					seen_slash = true;
				}
			}
			return true;
		}
		return path[1] == C(':') && path[2] == C('\\') && !path[3];
	}

	// points into the string. a trailing separator stays part of the name: "C:\dir\" gives "dir\"
	template <typename C>
	C *FindFileName(C *path)
	{
		if (!path)
			return nullptr;
		C *last = path;
		for (C *p = path; *p; p++)
		{
			if ((detail::IsSeparator(*p) || *p == C(':')) && p[1] && !detail::IsSeparator(p[1]))
				last = p + 1;
		}
		return last;
	}

	// points at the last '.', or at the terminator when there is no extension.
	// a backslash or a space after the '.' means it isn't an extension
	template <typename C>
	C *FindExtension(C *path)
	{
		if (!path)
			return nullptr;
		C *dot = nullptr;
		C *p = path;
		for (; *p; p++)
		{
			if (*p == C('\\') || *p == C(' '))
				dot = nullptr;
			else if (*p == C('.'))
				dot = p;
		}
		return dot ? dot : p;
	}

	template <typename C>
	C *FindNextComponent(C *path)
	{
		if (!path || !*path)
			return nullptr;
		C *p = path;
		for (; *p; p++)
		{
			if (*p == C('\\'))
				return p + (p[1] == C('\\') ? 2 : 1);
		}
		return p;
	}

	template <typename C>
	void StripPath(C *path)
	{
		if (path)
		{
			const C *name = FindFileName(path);
			detail::CopyTo<C>(path, detail::Length(name) + 1, name);
		}
	}

	template <typename C>
	void RemoveExtension(C *path)
	{
		if (path)
			*FindExtension(path) = C(0);
	}

	// returns a pointer to the terminator, or nullptr when the buffer is already full
	template <typename C>
	C *AddBackslash(C *path, size_t path_size = MaxLength)
	{
		if (!path)
			return nullptr;
		size_t length = detail::Length(path);
		if (length + 1 >= path_size)
			return nullptr;
		if (length && path[length - 1] != C('\\'))
		{
			path[length++] = C('\\');
			path[length] = C(0);
		}
		return path + length;
	}

	// a root keeps its backslash
	template <typename C>
	void RemoveBackslash(C *path)
	{
		if (!path || !*path)
			return;
		C *last = path + detail::Length(path) - 1;
		if (*last == C('\\') && !IsRoot(path))
			*last = C(0);
	}

	// removes the last component and the separators before it. a root keeps its backslash
	template <typename C>
	bool RemoveFileSpec(C *path)
	{
		if (!path)
			return false;
		C *p = path;
		C *cut = path;
		if (*p == C('\\'))
			cut = ++p;
		if (*p == C('\\'))
			cut = ++p;
		if (p == path && *p && p[1] == C(':'))
		{
			p += 2;
			if (*p == C('\\'))
				p++;
			cut = p;
		}
		for (; *p; p++)
		{
			if (*p == C('\\') && (p == path || p[-1] != C('\\')))
				cut = p;
		}
		if (!*cut)
			return false;
		*cut = C(0);
		return true;
	}

	template <typename C>
	bool StripToRoot(C *path)
	{
		if (!path)
			return false;
		if (*path && path[1] == C(':'))
		{
			path[path[2] == C('\\') ? 3 : 2] = C(0);
			return true;
		}
		while (!IsRoot(path))
		{
			if (!RemoveFileSpec(path))
				return false;
		}
		return true;
	}

	// resolves "." and ".." without touching the disk. dest may be the same buffer as path.
	// matches PathCanonicalize, including its handling of relative paths: "a\..\b" gives "\b"
	template <typename C>
	bool Canonicalize(C *dest, std::type_identity_t<const C *> path, size_t dest_size = MaxLength)
	{
		if (!dest || !path)
			return false;

		const auto is_server_share = [](const C *s) {
			if (s[0] != C('\\') || s[1] != C('\\'))
				return false;
			bool seen_slash = false;
			for (s += 2; *s; s++)
			{
				if (*s == C('\\'))
				{
					if (seen_slash)
						return false;
					seen_slash = true;
				}
			}
			return seen_slash;
		};

		std::basic_string<C> buffer(detail::Length(path) + 3, C(0));
		C *const start = buffer.data();
		C *out = start;
		const C *src = path;
		if (!*src)
		{
			*out++ = C('\\');
		}
		else
		{
			if (*src == C('\\'))
			{
				*out++ = *src++;
			}
			else if (src[1] == C(':'))
			{
				*out++ = *src++;
				*out++ = *src++;
				if (*src == C('\\'))
					*out++ = *src++;
			}

			while (*src)
			{
				if (*src != C('.'))
				{
					*out++ = *src++;
				}
				else if ((!src[1] || src[1] == C('\\')) && (src == path || src[-1] == C('\\') || src[-1] == C(':')))
				{
					// a "." component goes, and so does the separator before it at the end of the path
					src += src[1] ? 2 : 1;
					if (!*src && out > start + 1 && out[-1] == C('\\') && out[-2] != C(':') && out[-2] != C('\\'))
						out--;
				}
				else if (src[1] == C('.') && (!src[2] || src[2] == C('\\')) && (out == start || out[-1] == C('\\')))
				{
					// ".." backs up a component, except that it won't remove a UNC server name or the leading "\\"
					src += 2;
					if (out == start)
					{
						if (*src)
							src++;
					}
					else
					{
						*out = C(0);
						if (out > start + 1 && out[-1] == C('\\') && (out[-2] != C('\\') || out > start + 2))
						{
							if (out[-2] == C(':') && (out > start + 3 || out[-3] == C(':')))
							{
								out -= 2;
								while (out > start && *out != C('\\'))
									out--;
								out = *out == C('\\') ? out + 1 : start;
							}
							else if (out[-2] != C(':') && !is_server_share(start))
							{
								out -= 2;
							}
						}
						while (out > start && *out != C('\\'))
							out--;
						if (out == start)
						{
							*out++ = C('\\');
							if (*src)
								src++;
						}
					}
				}
				else
				{
					*out++ = *src++;
				}
			}

			// a name can't end in a dot, and "dir\.." leaves a separator it didn't end on
			while (out > start && out[-1] == C('.'))
				out--;
			if (out > start + 1 && out[-1] == C('\\') && out[-2] != C(':') && out[-2] != C('\\') && path[detail::Length(path) - 1] != C('\\'))
				out--;
			if (out - start == 2 && out[-1] == C(':'))
				*out++ = C('\\');
			if (out == start)
				*out++ = C('\\');
		}

		const size_t length = out - start;
		if (length >= dest_size)
		{
			*dest = C(0);
			return false;
		}
		std::char_traits<C>::copy(dest, start, length);
		dest[length] = C(0);
		return true;
	}

	// joins and canonicalizes. an absolute file ignores dir, and "\file" keeps only dir's root.
	// dest may be the same buffer as dir or file
	template <typename C>
	C *Combine(C *dest, std::type_identity_t<const C *> dir, std::type_identity_t<const C *> file, size_t dest_size = MaxLength)
	{
		if (!dest)
			return nullptr;
		if (!dir && !file)
		{
			*dest = C(0);
			return nullptr;
		}

		std::basic_string<C> joined;
		const bool has_dir = dir && *dir;
		const bool has_file = file && *file;
		if (!has_file)
		{
			if (has_dir)
				joined = dir;
		}
		else if (!has_dir || (!IsRelative(file) && (*file != C('\\') || IsUnc(file))))
		{
			joined = file;
		}
		else if (*file == C('\\'))
		{
			// only rooted directories can take a "\file"
			joined = dir;
			joined.resize(joined.size() + 1);
			if (!StripToRoot(joined.data()))
			{
				*dest = C(0);
				return nullptr;
			}
			joined.resize(detail::Length(joined.data()));
			if (!joined.empty() && joined.back() == C('\\'))
				joined.pop_back();
			joined += file;
		}
		else
		{
			joined = dir;
			if (joined.back() != C('\\'))
				joined.push_back(C('\\'));
			joined += file;
		}

		if (joined.size() + 1 >= dest_size || !Canonicalize(dest, joined.c_str(), dest_size))
		{
			*dest = C(0);
			return nullptr;
		}
		return dest;
	}

	template <typename C>
	bool Append(C *path, std::type_identity_t<const C *> more, size_t path_size = MaxLength)
	{
		if (!path || !more)
			return false;
		// PathAppend treats a leading separator on the addition as relative
		const bool unc = IsUnc(more);
		if (!unc)
			while (*more == C('\\'))
				more++;
		return Combine(path, path, more, path_size) != nullptr;
	}

	template <typename C>
	bool AddExtension(C *path, std::type_identity_t<const C *> extension, size_t path_size = MaxLength)
	{
		if (!path || *FindExtension(path))
			return false;
		const size_t length = detail::Length(path);
		const size_t extra = detail::Length(extension);
		if (length + extra >= path_size)
			return false;
		std::char_traits<C>::copy(path + length, extension, extra + 1);
		return true;
	}

	// wraps a path containing spaces in double quotes
	template <typename C>
	void QuoteSpaces(C *path, size_t path_size = MaxLength)
	{
		if (!path)
			return;
		const std::basic_string_view<C> view(path);
		if (view.find(C(' ')) == view.npos || view.size() + 2 >= path_size)
			return;
		std::char_traits<C>::move(path + 1, path, view.size());
		const size_t length = view.size();
		path[0] = C('"');
		path[length + 1] = C('"');
		path[length + 2] = C(0);
	}

	template <typename C>
	void RemoveBlanks(C *path)
	{
		if (!path)
			return;
		std::basic_string_view<C> view(path);
		const size_t first = view.find_first_not_of(C(' '));
		if (first == view.npos)
		{
			*path = C(0);
			return;
		}
		view = view.substr(first, view.find_last_not_of(C(' ')) - first + 1);
		detail::CopyTo<C>(path, view.size() + 1, view);
	}

	// the length of the shared leading whole components, ignoring case. out may be nullptr
	template <typename C>
	size_t CommonPrefix(const C *path1, const C *path2, C *out)
	{
		if (out)
			*out = C(0);
		if (!path1 || !path2)
			return 0;

		const C *p1 = path1;
		const C *p2 = path2;
		if (IsUnc(path1))
		{
			if (!IsUnc(path2))
				return 0;
			p1 += 2;
			p2 += 2;
		}
		else if (IsUnc(path2))
		{
			return 0;
		}

		size_t length = 0;
		for (;;)
		{
			if ((!*p1 || *p1 == C('\\')) && (!*p2 || *p2 == C('\\')))
				length = p1 - path1;
			if (!*p1 || detail::Fold(*p1) != detail::Fold(*p2))
				break;
			p1++;
			p2++;
		}
		// "C:" is the common root "C:\". callers compare this against a directory's length, so it matters
		if (length == 2)
			length = 3;
		if (out)
			detail::CopyTo<C>(out, length + 1, std::basic_string_view<C>(path1, length));
		return length;
	}

	template <typename C>
	bool IsSameRoot(const C *path1, const C *path2)
	{
		if (!path1 || !path2)
			return false;

		size_t root;
		if (path1[0] && path1[1] == C(':') && path1[2] == C('\\'))
		{
			root = 3;
		}
		else if (path1[0] == C('\\') && path1[1] != C('\\'))
		{
			root = 1;
		}
		else if (IsUnc(path1))
		{
			const std::basic_string_view<C> view(path1);
			const size_t server = view.find(C('\\'), 2);
			if (server == view.npos)
				root = view.size();
			else
			{
				const size_t share = view.find(C('\\'), server + 1);
				root = share == view.npos ? view.size() : share + 1;
			}
		}
		else
		{
			return false;
		}
		return CommonPrefix(path1, path2, static_cast<C *>(nullptr)) + 1 >= root;
	}

	// '*' and '?' wildcards, ignoring case. "*.*" also matches names with no dot
	inline bool MatchesMask(std::wstring_view name, std::wstring_view mask)
	{
		if (mask == L"*.*")
			return true;

		size_t n = 0, m = 0, star = std::wstring_view::npos, resume = 0;
		while (n < name.size())
		{
			if (m < mask.size() && mask[m] == L'*')
			{
				star = m++;
				resume = n;
			}
			else if (m < mask.size() && (mask[m] == L'?' || detail::Fold(mask[m]) == detail::Fold(name[n])))
			{
				m++;
				n++;
			}
			else if (star != std::wstring_view::npos)
			{
				m = star + 1;
				n = ++resume;
			}
			else
			{
				return false;
			}
		}
		while (m < mask.size() && mask[m] == L'*')
			m++;
		return m == mask.size();
	}

	// a snapshot of the matching entries, so the caller can safely delete or rename as it goes.
	// a missing or unreadable directory gives an empty list
	inline std::vector<std::filesystem::directory_entry> List(const std::filesystem::path &directory, std::wstring_view mask = L"*")
	{
		std::vector<std::filesystem::directory_entry> entries;
		std::error_code error;
		std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, error);
		const std::filesystem::directory_iterator end;
		for (; !error && it != end; it.increment(error))
		{
			if (MatchesMask(it->path().filename().native(), mask))
				entries.push_back(*it);
		}
		return entries;
	}

	// constructing a std::filesystem::path from a null pointer crashes, and callers of the Win32
	// functions these replace were allowed to pass one. so the common queries also take raw pointers
	inline bool Exists(const std::filesystem::path &path)
	{
		std::error_code error;
		return std::filesystem::exists(path, error);
	}
	inline bool Exists(const wchar_t *path) { return path && Exists(std::filesystem::path(path)); }
	inline bool Exists(const char *path) { return path && Exists(std::filesystem::path(path)); }

	inline bool IsDirectory(const std::filesystem::path &path)
	{
		std::error_code error;
		return std::filesystem::is_directory(path, error);
	}
	inline bool IsDirectory(const wchar_t *path) { return path && IsDirectory(std::filesystem::path(path)); }
	inline bool IsDirectory(const char *path) { return path && IsDirectory(std::filesystem::path(path)); }

	inline bool IsReadOnly(const std::filesystem::path &path)
	{
		std::error_code error;
		const auto status = std::filesystem::status(path, error);
		return !error && (status.permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none;
	}
	inline bool IsReadOnly(const wchar_t *path) { return path && IsReadOnly(std::filesystem::path(path)); }

	inline bool Remove(const std::filesystem::path &path)
	{
		std::error_code error;
		return std::filesystem::remove(path, error);
	}
	inline bool Remove(const wchar_t *path) { return path && Remove(std::filesystem::path(path)); }
	inline bool Remove(const char *path) { return path && Remove(std::filesystem::path(path)); }

	// C stdio can't open a wide path on Windows, so this is the one place that has to know about it.
	// _wfopen rather than _wfopen_s, which would stop other programs from opening the file
	inline std::FILE *Open(const std::filesystem::path &path, const wchar_t *mode)
	{
#ifdef _WIN32
#pragma warning(suppress: 4996)
		return _wfopen(path.c_str(), mode);
#else
		return std::fopen(path.c_str(), std::filesystem::path(mode).string().c_str());
#endif
	}
	inline std::FILE *Open(const wchar_t *path, const wchar_t *mode) { return path ? Open(std::filesystem::path(path), mode) : nullptr; }

	inline bool Rename(const std::filesystem::path &from, const std::filesystem::path &to)
	{
		std::error_code error;
		std::filesystem::rename(from, to, error);
		return !error;
	}

	inline bool Copy(const std::filesystem::path &from, const std::filesystem::path &to, bool overwrite)
	{
		std::error_code error;
		std::filesystem::copy_file(from, to, overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none, error);
		return !error;
	}

	inline std::filesystem::path TempDirectory()
	{
		std::error_code error;
		std::filesystem::path directory = std::filesystem::temp_directory_path(error);
		return error ? std::filesystem::path(L".") : directory;
	}

	// an unused name in directory. nothing is created, so the caller makes the file or directory
	inline std::filesystem::path UniqueName(const std::filesystem::path &directory, std::wstring_view prefix, std::wstring_view extension = L".tmp")
	{
		static constexpr wchar_t digits[] = L"0123456789abcdef";
		std::random_device random;
		for (;;)
		{
			uint64_t value = (static_cast<uint64_t>(random()) << 32) | random();
			std::wstring name(prefix);
			for (int i = 0; i < 16; i++, value >>= 4)
				name.push_back(digits[value & 15]);
			name += extension;
			std::filesystem::path candidate = directory / name;
			if (!Exists(candidate))
				return candidate;
		}
	}

	// false if the file can't be opened or read in full
	inline bool ReadAll(const std::filesystem::path &path, std::vector<char> &contents)
	{
		std::error_code error;
		const auto size = std::filesystem::file_size(path, error);
		if (error)
			return false;

		std::ifstream file(path, std::ios::binary);
		if (!file)
			return false;

		contents.resize(static_cast<size_t>(size));
		return size == 0 || static_cast<bool>(file.read(contents.data(), static_cast<std::streamsize>(contents.size())));
	}
}
