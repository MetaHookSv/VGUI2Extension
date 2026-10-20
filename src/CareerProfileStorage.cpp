#include "CareerProfileStorage.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace CareerProfileStorage
{
namespace
{
constexpr std::string_view KeyPrefix     = "u8hex-";
constexpr std::string_view ProfilePrefix = "CZProfile-";
constexpr std::string_view ProfileSuffix = ".czp";
constexpr char             HexDigits[]   = "0123456789abcdef";

template <typename T>
Result<T> Failure(Error error)
{
    return {{}, error};
}

unsigned char AsciiLower(unsigned char value)
{
    return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
}

bool SameWindowsPath(std::string_view left, std::string_view right)
{
    if (left.size() != right.size())
        return false;
    for (size_t i = 0; i < left.size(); ++i)
        if (AsciiLower(static_cast<unsigned char>(left[i])) != AsciiLower(static_cast<unsigned char>(right[i])))
            return false;
    return true;
}

Result<std::string> NormalizeDirectory(std::string_view input)
{
    if (input.empty())
        return Failure<std::string>(Error::InvalidDirectory);
    std::string directory(input);
    std::replace(directory.begin(), directory.end(), '\\', '/');
    for (unsigned char byte : directory)
        if (byte < 32 || byte == 127 || std::string_view(":\"<>|?*").find(static_cast<char>(byte)) != std::string_view::npos)
            return Failure<std::string>(Error::InvalidDirectory);
    size_t start = 0;
    for (;;)
    {
        auto end = directory.find('/', start);
        if (end == std::string::npos)
            end = directory.size();
        const std::string_view part(directory.data() + start, end - start);
        if (part.empty() || part == "." || part == ".." || part.back() == '.' || part.back() == ' ')
            return Failure<std::string>(Error::InvalidDirectory);
        //Win32 device aliases can bypass an apparently relative path.
        const auto stem = part.substr(0, part.find('.'));
        if (SameWindowsPath(stem, "CON") || SameWindowsPath(stem, "PRN") ||
            SameWindowsPath(stem, "AUX") || SameWindowsPath(stem, "NUL") ||
            (stem.size() == 4 && stem[3] >= '1' && stem[3] <= '9' &&
             (SameWindowsPath(stem.substr(0, 3), "COM") || SameWindowsPath(stem.substr(0, 3), "LPT"))))
            return Failure<std::string>(Error::InvalidDirectory);
        if (end == directory.size())
            break;
        start = end + 1;
    }
    return {std::move(directory)};
}

bool Fits(size_t length, size_t capacity)
{
    return length < capacity;
}

bool FitsPhysical(size_t prefix, size_t relative, size_t capacity)
{
    //Prefix + separator + relative + NUL, without overflowing size_t.
    return prefix < capacity && relative < capacity - prefix && relative + 1 < capacity - prefix;
}

Error ObservationError(const ObservedProfile& observed)
{
    if (observed.state == FileState::Unreadable)
        return Error::ReadFailed;
    if (observed.state == FileState::Invalid)
        return observed.error == Error::None ? Error::MalformedProfile : observed.error;
    return observed.state == FileState::Missing ? Error::NotFound : Error::None;
}

class ProfileReader
{
public:
    explicit ProfileReader(std::string_view payload) : bytes(payload) {}

    bool Skip(size_t count)
    {
        if (count > Remaining())
        {
            valid = false;
            return false;
        }
        position += count;
        return true;
    }

    uint32_t Byte()
    {
        if (!Skip(1))
            return 0;
        return static_cast<unsigned char>(bytes[position - 1]);
    }

    uint32_t Word()
    {
        if (!Skip(4))
            return 0;
        uint32_t result = 0;
        for (unsigned shift = 0; shift < 32; shift += 8)
            result |= static_cast<uint32_t>(static_cast<unsigned char>(bytes[position - 4 + shift / 8])) << shift;
        return result;
    }

    std::string_view String()
    {
        const size_t start = position;
        //ReadString's 2048-byte temporary exits before consuming NUL when
        //there are 2047 content bytes. Require an unambiguous native read.
        while (position < bytes.size() && position - start < ReaderStringCapacity)
        {
            const auto byte = static_cast<unsigned char>(bytes[position++]);
            if (byte == 0)
                return bytes.substr(start, position - start - 1);
            if (byte == 255)
                break;
        }
        valid = false;
        return {};
    }

    bool Difficulty(uint32_t version)
    {
        if (!Byte())
            return valid;
        Byte();
        const auto bots = Byte();
        if (!valid || bots > 5)
            return false;
        for (uint32_t i = 0; i < bots; ++i)
            String();
        const auto maps = Byte();
        if (!valid || maps < 3 || maps % 3 != 0)
            return false;
        for (uint32_t i = 0; i < maps && valid; ++i)
        {
            String();
            Byte();
        }
        String();
        if (version >= 3)
            Byte();
        return valid;
    }

    size_t Remaining() const { return bytes.size() - position; }
    bool   Valid() const { return valid; }

private:
    std::string_view bytes;
    size_t           position = 0;
    bool             valid    = true;
};
} //namespace

const char* DescribeError(Error error) noexcept
{
    switch (error)
    {
        case Error::None: return "No error.";
        case Error::EmptyName: return "The career player name is empty.";
        case Error::InvalidUtf8: return "The career player name is not valid UTF-8.";
        case Error::UnsafeName: return "The player name contains a character unsafe for native commands.";
        case Error::InvalidKey: return "The career storage key is not canonical lowercase UTF-8 hex.";
        case Error::InvalidDirectory: return "The mission pack save directory is not a safe relative path.";
        case Error::UnverifiedCapacity: return "A required native or physical path capacity has not been verified.";
        case Error::CapacityExceeded: return "The complete name or path exceeds a verified buffer capacity.";
        case Error::MalformedProfile: return "The career profile has an invalid or incomplete structure.";
        case Error::UnsupportedVersion: return "The career profile version is not supported.";
        case Error::ChecksumMismatch: return "The career profile checksum does not match.";
        case Error::ReadFailed: return "The career file exists but could not be read.";
        case Error::OpenFailed: return "The career profile could not be opened for writing.";
        case Error::ShortWrite: return "The complete career profile could not be written.";
        case Error::FlushFailed: return "The career profile could not be flushed successfully.";
        case Error::DeleteFailed: return "The verified career profile could not be deleted.";
        case Error::CoverageMismatch: return "The native career operation did not complete its verified I/O sequence.";
        case Error::NotFound: return "No career profile with this complete player name was found.";
        case Error::IdentityConflict: return "Another player owns this path. Back up and explicitly migrate the conflicting file.";
        case Error::InvalidIdentity: return "The career identity does not match its canonical path.";
        case Error::WrongMissionPack: return "The career identity belongs to another mission pack.";
        case Error::AmbiguousFiles: return "Multiple career files occupy the same Windows path.";
        case Error::IncompleteIndex: return "The mission pack profile scan did not complete; storage changes are blocked.";
        case Error::StalePanel: return "The career panel or its owning frame no longer exists.";
        case Error::StaleSelection: return "The career selection is missing or belongs to an older profile list.";
        case Error::StaleConfirmation: return "The career confirmation is expired or belongs to another operation.";
    }
    return "Unknown career storage error.";
}

Status ValidateName(std::string_view rawUtf8, size_t capacity)
{
    if (!capacity)
        return {Error::UnverifiedCapacity};
    if (rawUtf8.empty())
        return {Error::EmptyName};
    if (!Fits(rawUtf8.size(), capacity))
        return {Error::CapacityExceeded};
    for (size_t i = 0; i < rawUtf8.size();)
    {
        const auto first = static_cast<unsigned char>(rawUtf8[i++]);
        uint32_t   code  = first;
        uint32_t   min   = 0;
        size_t     tail  = 0;
        if (first >= 0xc2 && first <= 0xdf)
        {
            code = first & 0x1f;
            min  = 0x80;
            tail = 1;
        }
        else if (first >= 0xe0 && first <= 0xef)
        {
            code = first & 0x0f;
            min  = 0x800;
            tail = 2;
        }
        else if (first >= 0xf0 && first <= 0xf4)
        {
            code = first & 7;
            min  = 0x10000;
            tail = 3;
        }
        else if (first >= 0x80)
            return {Error::InvalidUtf8};
        if (tail > rawUtf8.size() - i)
            return {Error::InvalidUtf8};
        for (size_t j = 0; j < tail; ++j)
        {
            const auto next = static_cast<unsigned char>(rawUtf8[i++]);
            if ((next & 0xc0) != 0x80)
                return {Error::InvalidUtf8};
            code = (code << 6) | (next & 0x3f);
        }
        if (code < min || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
            return {Error::InvalidUtf8};
        if (code < 32 || (code >= 127 && code <= 159) || code == '"' || code == ';' || code == '\\')
            return {Error::UnsafeName};
    }
    return {};
}

Result<std::string> EncodeKey(std::string_view rawUtf8)
{
    const auto valid = ValidateName(rawUtf8);
    if (!valid)
        return Failure<std::string>(valid.error);
    std::string result(KeyPrefix);
    if (rawUtf8.size() > (result.max_size() - KeyPrefix.size()) / 2)
        return Failure<std::string>(Error::CapacityExceeded);
    result.reserve(KeyPrefix.size() + 2 * rawUtf8.size());
    for (unsigned char byte : rawUtf8)
    {
        result.push_back(HexDigits[byte >> 4]);
        result.push_back(HexDigits[byte & 15]);
    }
    return {std::move(result)};
}

Result<std::string> DecodeKey(std::string_view key)
{
    if (key.substr(0, KeyPrefix.size()) != KeyPrefix || key.size() <= KeyPrefix.size() ||
        (key.size() - KeyPrefix.size()) % 2 != 0)
        return Failure<std::string>(Error::InvalidKey);
    const auto length = (key.size() - KeyPrefix.size()) / 2;
    if (!Fits(length, ReaderStringCapacity))
        return Failure<std::string>(Error::CapacityExceeded);
    std::string raw;
    raw.reserve(length);
    for (size_t i = KeyPrefix.size(); i < key.size(); i += 2)
    {
        const auto high = std::string_view(HexDigits, 16).find(key[i]);
        const auto low  = std::string_view(HexDigits, 16).find(key[i + 1]);
        if (high == std::string_view::npos || low == std::string_view::npos)
            return Failure<std::string>(Error::InvalidKey);
        raw.push_back(static_cast<char>(high * 16 + low));
    }
    const auto encoded = EncodeKey(raw);
    if (!encoded || encoded.value != key)
        return Failure<std::string>(Error::InvalidKey);
    return {std::move(raw)};
}

std::string LegacyKey(std::string_view rawUtf8)
{
    std::string key(rawUtf8.substr(0, LegacyKeyCapacity - 1));
    const auto  nul = key.find('\0');
    if (nul != std::string::npos)
        key.resize(nul);
    for (char& byte : key)
        if (!((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
              (byte >= '0' && byte <= '9') || byte == '_' || byte == '-'))
            byte = '_';
    return key;
}

std::string WindowsPathKey(std::string_view path)
{
    std::string result(path);
    for (char& byte : result)
        byte = byte == '\\' ? '/' : static_cast<char>(AsciiLower(static_cast<unsigned char>(byte)));
    return result;
}

Result<Identity> BuildIdentity(std::string_view rawUtf8, std::string_view saveDirectory, const PathLimits& limits)
{
    if (!limits.rawNameCapacity || !limits.profilePathCapacity || !limits.enumeratedPathCapacity ||
        !limits.writeCommandCapacity || !limits.execCommandCapacity || !limits.systemPathCapacity ||
        !limits.physicalGameDirectoryBytes)
        return Failure<Identity>(Error::UnverifiedCapacity);
    auto valid = ValidateName(rawUtf8, limits.rawNameCapacity);
    if (!valid)
        return Failure<Identity>(valid.error);
    auto encoded = EncodeKey(rawUtf8);
    if (!encoded)
        return Failure<Identity>(encoded.error);
    auto normalized = NormalizeDirectory(saveDirectory);
    if (!normalized)
        return Failure<Identity>(normalized.error);
    Identity identity;
    identity.rawUtf8            = rawUtf8;
    identity.storageKey         = std::move(encoded.value);
    identity.saveDirectory      = std::move(normalized.value);
    identity.profilePath        = identity.saveDirectory + "/CZProfile-" + identity.storageKey + ".czp";
    identity.configPath         = "Save/" + identity.storageKey + ".cfg";
    identity.writeConfigCommand = "writecfg Save/" + identity.storageKey + "\n";
    identity.execConfigCommand  = "wait\nexec " + identity.configPath + "\n";
    if (!Fits(identity.profilePath.size(), limits.profilePathCapacity) ||
        !Fits(identity.profilePath.size(), limits.enumeratedPathCapacity) ||
        !Fits(identity.writeConfigCommand.size(), limits.writeCommandCapacity) ||
        !Fits(identity.execConfigCommand.size(), limits.execCommandCapacity) ||
        !FitsPhysical(limits.physicalGameDirectoryBytes, identity.profilePath.size(), limits.systemPathCapacity) ||
        !FitsPhysical(limits.physicalGameDirectoryBytes, identity.configPath.size(), limits.systemPathCapacity))
        return Failure<Identity>(Error::CapacityExceeded);
    return {std::move(identity)};
}

Result<ProfileInfo> InspectProfile(std::string_view bytes)
{
    if (bytes.size() > ProfileBufferCapacity)
        return Failure<ProfileInfo>(Error::CapacityExceeded);
    constexpr size_t ChecksumBytes = 4;
    if (bytes.size() < 4 + 4 + 1 + ChecksumBytes || bytes.substr(0, 4) != std::string_view("CZP\0", 4))
        return Failure<ProfileInfo>(Error::MalformedProfile);
    const auto    payload = bytes.substr(0, bytes.size() - ChecksumBytes);
    ProfileReader checksumReader(bytes.substr(payload.size()));
    uint32_t      sum = 0;
    for (size_t i = 0; i < payload.size(); ++i)
        sum += static_cast<uint32_t>(i + 1) * static_cast<unsigned char>(payload[i]);
    if (sum != checksumReader.Word())
        return Failure<ProfileInfo>(Error::ChecksumMismatch);

    ProfileReader reader(payload);
    reader.Skip(4);
    const auto version = reader.Word();
    if (version < 1 || version > 4)
        return Failure<ProfileInfo>(Error::UnsupportedVersion);
    const auto raw = reader.String();
    if (!reader.Valid())
        return Failure<ProfileInfo>(Error::MalformedProfile);
    const auto valid = ValidateName(raw);
    if (!valid)
        return Failure<ProfileInfo>(valid.error);
    reader.Word(); //botPoolSliderPosition
    if (version >= 4)
        reader.Byte(); //Native parser clamps lastDifficulty outside 0..3.
    reader.Skip(4);    //medals
    for (int difficulty = 0; difficulty < 4; ++difficulty)
        if (!reader.Difficulty(version))
            return Failure<ProfileInfo>(Error::MalformedProfile);
    if (version != 1)
    {
        const auto count = reader.Word();
        //Reject negative signed counts, even though native skips their loop.
        if (count > static_cast<uint32_t>((std::numeric_limits<int32_t>::max)()) || count > reader.Remaining() / 4)
            return Failure<ProfileInfo>(Error::MalformedProfile);
        reader.Skip(static_cast<size_t>(count) * 4);
    }
    if (!reader.Valid() || reader.Remaining())
        return Failure<ProfileInfo>(Error::MalformedProfile);
    return {{std::string(raw), version}};
}

Status NativeWriteAudit::Fail(Error next)
{
    if (error == Error::None) error = next;
    return {error};
}

Status NativeWriteAudit::Serialized(std::string_view bytes, std::string_view rawUtf8, size_t capacity, bool overflow)
{
    if (error != Error::None) return {error};
    if (length || opens) return Fail(Error::CoverageMismatch);
    if (overflow || capacity != ProfileBufferCapacity || bytes.empty() || bytes.size() > capacity)
        return Fail(Error::CapacityExceeded);
    auto inspected = InspectProfile(bytes);
    if (!inspected) return Fail(inspected.error);
    if (inspected.value.rawUtf8 != rawUtf8) return Fail(Error::InvalidIdentity);
    length = bytes.size();
    return {};
}

Status NativeWriteAudit::Opening()
{
    if (!length || opens++) return Fail(Error::CoverageMismatch);
    return {error};
}

Status NativeWriteAudit::Opened(bool success)
{
    if (opens != 1) return Fail(Error::CoverageMismatch);
    return success ? Status{error} : Fail(Error::OpenFailed);
}

Status NativeWriteAudit::Written(int requested, int actual)
{
    if (opens != 1 || closes || writes++ || requested < 0 || static_cast<size_t>(requested) != length)
        return Fail(Error::CoverageMismatch);
    return requested == actual ? Status{error} : Fail(Error::ShortWrite);
}

Status NativeWriteAudit::Closed(bool flushed)
{
    if (opens != 1 || closes++) return Fail(Error::CoverageMismatch);
    return flushed ? Status{error} : Fail(Error::FlushFailed);
}

Status NativeWriteAudit::Finish() const
{
    if (error != Error::None) return {error};
    return {length && opens == 1 && writes == 1 && closes == 1 ? Error::None : Error::CoverageMismatch};
}

ProfileIndex::ProfileIndex(std::string_view saveDirectory)
{
    auto normalized = NormalizeDirectory(saveDirectory);
    directory       = std::move(normalized.value);
    indexError      = normalized.error;
}

Status ProfileIndex::Add(ObservedProfile observed)
{
    scanComplete = false;
    if (observed.state == FileState::Missing)
        return {};
    const auto& filename = observed.filename;
    if (filename.size() <= ProfilePrefix.size() + ProfileSuffix.size() ||
        !SameWindowsPath(std::string_view(filename).substr(0, ProfilePrefix.size()), ProfilePrefix) ||
        !SameWindowsPath(std::string_view(filename).substr(filename.size() - ProfileSuffix.size()), ProfileSuffix) ||
        filename.find_first_of("/\\\0", 0, 3) != std::string::npos)
    {
        MarkIncomplete(Error::MalformedProfile, filename);
        return {Error::MalformedProfile};
    }
    if (Find(directory + "/" + filename))
    {
        diagnostics.push_back({Error::AmbiguousFiles, directory + "/" + filename});
        indexError = Error::AmbiguousFiles;
        return {indexError};
    }
    bool canonical = false;
    if (observed.state == FileState::Present)
    {
        const auto key     = std::string_view(filename).substr(ProfilePrefix.size(), filename.size() - ProfilePrefix.size() - ProfileSuffix.size());
        const auto encoded = EncodeKey(observed.profile.rawUtf8);
        canonical          = encoded && key == encoded.value;
        if (!canonical && (!encoded || !SameWindowsPath(key, LegacyKey(observed.profile.rawUtf8))))
        {
            observed.state = FileState::Invalid;
            observed.error = encoded ? Error::InvalidIdentity : encoded.error;
        }
    }
    const auto error = ObservationError(observed);
    if (error != Error::None)
        diagnostics.push_back({error, directory + "/" + filename});
    records.push_back({std::move(observed), canonical});
    return {error};
}

Status ProfileIndex::FinishScan()
{
    if (indexError != Error::None)
        return {indexError};
    scanComplete = true;
    return {};
}

void ProfileIndex::MarkIncomplete(Error error, std::string_view path)
{
    scanComplete = false;
    indexError   = Error::IncompleteIndex;
    diagnostics.push_back({error, std::string(path)});
}

Error ProfileIndex::CheckIdentity(const Identity& identity) const
{
    if (indexError != Error::None)
        return indexError;
    if (!scanComplete)
        return Error::IncompleteIndex;
    if (identity.saveDirectory != directory)
        return Error::WrongMissionPack;
    const auto key = EncodeKey(identity.rawUtf8);
    if (!key || identity.storageKey != key.value || identity.profilePath != directory + "/CZProfile-" + key.value + ".czp")
        return Error::InvalidIdentity;
    return Error::None;
}

const ProfileIndex::Record* ProfileIndex::Find(std::string_view path) const
{
    for (const auto& record : records)
        if (SameWindowsPath(directory + "/" + record.observed.filename, path))
            return &record;
    return nullptr;
}

Result<std::string> ProfileIndex::Resolve(std::string_view rawUtf8, std::string_view canonicalPath) const
{
    if (const auto* canonical = Find(canonicalPath))
    {
        const auto error = ObservationError(canonical->observed);
        if (error != Error::None)
            return Failure<std::string>(error);
        if (canonical->observed.profile.rawUtf8 != rawUtf8)
            return Failure<std::string>(Error::IdentityConflict);
        return {directory + "/" + canonical->observed.filename};
    }
    for (const auto& record : records)
        if (record.observed.state == FileState::Present && record.observed.profile.rawUtf8 == rawUtf8)
            return {directory + "/" + record.observed.filename};
    //A failed candidate read is not evidence that the requested legacy identity
    //does not exist. Never turn a permissions/corruption error into absence.
    if (const auto* legacy = Find(directory + "/CZProfile-" + LegacyKey(rawUtf8) + ".czp"))
    {
        const auto error = ObservationError(legacy->observed);
        if (error != Error::None)
            return Failure<std::string>(error);
    }
    return Failure<std::string>(Error::NotFound);
}

Result<std::string> ProfileIndex::ResolveRead(const Identity& identity) const
{
    const auto error = CheckIdentity(identity);
    if (error != Error::None)
        return Failure<std::string>(error);
    return Resolve(identity.rawUtf8, identity.profilePath);
}

Status ProfileIndex::CheckWrite(const Identity& identity) const
{
    const auto error = CheckIdentity(identity);
    if (error != Error::None)
        return {error};
    if (const auto* target = Find(identity.profilePath))
    {
        const auto fileError = ObservationError(target->observed);
        if (fileError != Error::None)
            return {fileError};
        if (target->observed.profile.rawUtf8 != identity.rawUtf8)
            return {Error::IdentityConflict};
    }
    return {};
}

Result<std::set<std::string>> ProfileIndex::PlanDelete(const Identity& identity) const
{
    const auto writable = CheckWrite(identity);
    if (!writable)
        return Failure<std::set<std::string>>(writable.error);
    if (const auto* legacy = Find(directory + "/CZProfile-" + LegacyKey(identity.rawUtf8) + ".czp"))
    {
        const auto error = ObservationError(legacy->observed);
        if (error != Error::None)
            return Failure<std::set<std::string>>(error);
    }
    std::set<std::string> paths;
    for (const auto& record : records)
        if (record.observed.state == FileState::Present && record.observed.profile.rawUtf8 == identity.rawUtf8)
            paths.insert(directory + "/" + record.observed.filename);
    if (paths.empty())
        return Failure<std::set<std::string>>(Error::NotFound);
    return {std::move(paths)};
}

Result<std::vector<ProfileEntry>> ProfileIndex::Enumerate() const
{
    if (indexError != Error::None)
        return Failure<std::vector<ProfileEntry>>(indexError);
    if (!scanComplete)
        return Failure<std::vector<ProfileEntry>>(Error::IncompleteIndex);
    std::vector<ProfileEntry> list;
    std::set<std::string>     included;
    for (const auto& record : records)
    {
        if (record.observed.state != FileState::Present)
            continue;
        const auto& raw = record.observed.profile.rawUtf8;
        const auto  key = EncodeKey(raw);
        if (!key)
            continue;
        const auto resolved = Resolve(raw, directory + "/CZProfile-" + key.value + ".czp");
        if (!resolved || !included.insert(raw).second)
            continue;
        const auto* chosen = Find(resolved.value);
        list.push_back({raw, resolved.value, chosen->observed.fileTime});
    }
    std::stable_sort(list.begin(), list.end(), [](const ProfileEntry& left, const ProfileEntry& right) {
        return left.fileTime > right.fileTime;
    });
    return {std::move(list)};
}

Result<std::string> ConfigCommand(const Identity& identity, FileState canonicalState)
{
    if (canonicalState == FileState::Missing)
        return {};
    if (canonicalState != FileState::Present)
        return Failure<std::string>(canonicalState == FileState::Unreadable ? Error::ReadFailed : Error::MalformedProfile);
    const auto encoded = EncodeKey(identity.rawUtf8);
    if (!encoded || identity.configPath != "Save/" + encoded.value + ".cfg" ||
        identity.execConfigCommand != "wait\nexec " + identity.configPath + "\n")
        return Failure<std::string>(Error::InvalidIdentity);
    return {identity.execConfigCommand};
}

thread_local OperationContext* OperationScope::current = nullptr;

OperationScope::OperationScope(OperationContext next) : context(std::move(next)), previous(current)
{
    current = &context;
}

OperationScope::~OperationScope()
{
    current = previous;
}

OperationContext* OperationScope::Current() noexcept
{
    return current;
}

IdentitySidecars::IdentitySidecars(std::function<bool(PanelHandle)> validator) : isLive(std::move(validator)) {}

const IdentitySidecars::Frame* IdentitySidecars::GetFrame(PanelHandle frame) const
{
    const auto found = frames.find(frame);
    return isLive && isLive(frame) && found != frames.end() ? &found->second : nullptr;
}

IdentitySidecars::Frame* IdentitySidecars::GetFrame(PanelHandle frame)
{
    return const_cast<Frame*>(static_cast<const IdentitySidecars*>(this)->GetFrame(frame));
}

Result<std::string> IdentitySidecars::NewToken()
{
    if (sequence == (std::numeric_limits<uint64_t>::max)())
        return Failure<std::string>(Error::CapacityExceeded);
    return {"cps" + std::to_string(++sequence)};
}

Status IdentitySidecars::BeginPopulate(PanelHandle frame, std::string_view saveDirectory)
{
    Forget(frame);
    if (!isLive || !isLive(frame))
        return {Error::StalePanel};
    auto normalized = NormalizeDirectory(saveDirectory);
    if (!normalized)
        return {normalized.error};
    Frame state;
    state.directory = std::move(normalized.value);
    frames.emplace(frame, std::move(state));
    return {};
}

Result<std::string> IdentitySidecars::AddButton(PanelHandle frame, PanelHandle button, std::string_view rawUtf8)
{
    auto* state = GetFrame(frame);
    if (!state || button == frame || !isLive(button))
        return Failure<std::string>(Error::StalePanel);
    const auto valid = ValidateName(rawUtf8);
    if (!valid)
        return Failure<std::string>(valid.error);
    auto token = NewToken();
    if (!token)
        return token;
    Forget(button);
    state->tokens.emplace(token.value, Binding{button, std::string(rawUtf8)});
    buttons[button] = {frame, std::string(rawUtf8)};
    return token;
}

Result<std::string> IdentitySidecars::ButtonName(PanelHandle button) const
{
    const auto found = buttons.find(button);
    if (!isLive || !isLive(button) || found == buttons.end() || !GetFrame(found->second.frame))
        return Failure<std::string>(Error::StalePanel);
    return {found->second.rawUtf8};
}

Result<std::string> IdentitySidecars::TokenForName(PanelHandle frame, std::string_view rawUtf8) const
{
    const auto* state = GetFrame(frame);
    if (!state)
        return Failure<std::string>(Error::StalePanel);
    for (const auto& token : state->tokens)
        if (token.second.rawUtf8 == rawUtf8 && isLive(token.second.button))
            return {token.first};
    return Failure<std::string>(Error::StaleSelection);
}

Status IdentitySidecars::SelectName(PanelHandle frame, std::string_view rawUtf8)
{
    const auto token = TokenForName(frame, rawUtf8);
    return token ? Select(frame, token.value) : Status{token.error};
}

std::vector<std::pair<PanelHandle, std::string>> IdentitySidecars::Buttons(PanelHandle frame) const
{
    std::vector<std::pair<PanelHandle, std::string>> result;
    if (GetFrame(frame))
        for (const auto& button : buttons)
            if (button.second.frame == frame && isLive(button.first))
                result.emplace_back(button.first, button.second.rawUtf8);
    return result;
}

Status IdentitySidecars::Select(PanelHandle frame, std::string_view token)
{
    auto* state = GetFrame(frame);
    if (!state)
        return {Error::StalePanel};
    const auto found = state->tokens.find(std::string(token));
    if (found == state->tokens.end() || !isLive(found->second.button))
        return {Error::StaleSelection};
    state->selected       = found->second.rawUtf8;
    state->selectedButton = found->second.button;
    state->confirmation.clear();
    state->confirmedName.clear();
    return {};
}

Result<std::string> IdentitySidecars::Selected(PanelHandle frame) const
{
    const auto* state = GetFrame(frame);
    if (!state)
        return Failure<std::string>(Error::StalePanel);
    if (state->selected.empty() || !isLive(state->selectedButton))
        return Failure<std::string>(Error::StaleSelection);
    return {state->selected};
}

Result<std::string> IdentitySidecars::Confirm(PanelHandle frame, Operation operation, std::string_view createName)
{
    auto* state = GetFrame(frame);
    if (!state)
        return Failure<std::string>(Error::StalePanel);
    if (operation != Operation::Create && operation != Operation::Delete)
        return Failure<std::string>(Error::StaleSelection);
    const auto raw = operation == Operation::Create ? Result<std::string>{std::string(createName)} : Selected(frame);
    if (!raw)
        return Failure<std::string>(raw.error);
    const auto valid = ValidateName(raw.value);
    if (!valid)
        return Failure<std::string>(valid.error);
    auto token = NewToken();
    if (!token)
        return token;
    state->confirmation       = token.value;
    state->confirmedName      = raw.value;
    state->confirmedOperation = operation;
    return token;
}

Result<SelectedIdentity> IdentitySidecars::ConsumeConfirmation(PanelHandle frame, std::string_view token, Operation operation)
{
    auto* state = GetFrame(frame);
    if (!state || token.empty() || state->confirmation != token || state->confirmedOperation != operation ||
        (operation == Operation::Delete && (!Selected(frame) || state->confirmedName != state->selected)))
        return Failure<SelectedIdentity>(Error::StaleConfirmation);
    SelectedIdentity identity;
    identity.rawUtf8       = state->confirmedName;
    identity.saveDirectory = state->directory;
    state->confirmation.clear();
    state->confirmedName.clear();
    return {std::move(identity)};
}

void IdentitySidecars::CancelConfirmation(PanelHandle frame)
{
    if (auto* state = GetFrame(frame))
    {
        state->confirmation.clear();
        state->confirmedName.clear();
    }
}

void IdentitySidecars::Forget(PanelHandle panel)
{
    frames.erase(panel);
    for (auto& frame : frames)
    {
        auto& state = frame.second;
        for (auto it = state.tokens.begin(); it != state.tokens.end();)
        {
            if (it->second.button == panel)
                it = state.tokens.erase(it);
            else
                ++it;
        }
        if (state.selectedButton == panel)
        {
            state.selected.clear();
            state.selectedButton = 0;
            state.confirmation.clear();
            state.confirmedName.clear();
        }
    }
    for (auto it = buttons.begin(); it != buttons.end();)
    {
        if (it->first == panel || it->second.frame == panel)
            it = buttons.erase(it);
        else
            ++it;
    }
}

void IdentitySidecars::Clear()
{
    frames.clear();
    buttons.clear();
    //Keep the token sequence monotonic across module-session resets.
}
} //namespace CareerProfileStorage
