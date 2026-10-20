#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

//Private storage policy. No GameUI objects, native allocators or filesystem ABI
//cross this boundary. Native adapters must catch exceptions before returning.
namespace CareerProfileStorage
{
enum class Error
{
    None,
    EmptyName,
    InvalidUtf8,
    UnsafeName,
    InvalidKey,
    InvalidDirectory,
    UnverifiedCapacity,
    CapacityExceeded,
    MalformedProfile,
    UnsupportedVersion,
    ChecksumMismatch,
    ReadFailed,
    OpenFailed,
    ShortWrite,
    FlushFailed,
    DeleteFailed,
    CoverageMismatch,
    NotFound,
    IdentityConflict,
    InvalidIdentity,
    WrongMissionPack,
    AmbiguousFiles,
    IncompleteIndex,
    StalePanel,
    StaleSelection,
    StaleConfirmation
};

const char* DescribeError(Error error) noexcept;

template <typename T>
struct Result
{
    T     value{};
    Error error = Error::None;

    explicit operator bool() const noexcept { return error == Error::None; }
};

struct Status
{
    Error error = Error::None;

    explicit operator bool() const noexcept { return error == Error::None; }
};

//Known Windows hl-10210 reader bounds; these are format limits, not permission
//to enable another binary identity or to assume a name fits every native path.
inline constexpr size_t ProfileBufferCapacity = 16384;
inline constexpr size_t ReaderStringCapacity  = 2047;
inline constexpr size_t LegacyKeyCapacity     = 32;

Status              ValidateName(std::string_view rawUtf8, size_t capacity = ReaderStringCapacity);
Result<std::string> EncodeKey(std::string_view rawUtf8);
Result<std::string> DecodeKey(std::string_view key);
std::string         LegacyKey(std::string_view rawUtf8);

struct PathLimits
{
    //All capacities include NUL. Zero means unverified and refuses an operation.
    size_t rawNameCapacity;
    size_t profilePathCapacity;
    size_t enumeratedPathCapacity;
    size_t writeCommandCapacity;
    size_t execCommandCapacity;
    size_t systemPathCapacity;
    //Bytes in the resolved physical game-directory prefix, excluding its final
    //separator. Obtain this from the actual filesystem/search path before I/O.
    size_t physicalGameDirectoryBytes;
};

struct Identity
{
    std::string rawUtf8;
    std::string storageKey;
    std::string saveDirectory;
    std::string profilePath;
    std::string configPath;
    std::string writeConfigCommand;
    std::string execConfigCommand;
};

Result<Identity> BuildIdentity(std::string_view rawUtf8, std::string_view saveDirectory, const PathLimits& limits);
std::string      WindowsPathKey(std::string_view path);

struct ProfileInfo
{
    std::string rawUtf8;
    uint32_t    version = 0;
};

//Independent bounded reader for hl-10210's v1..v4 format. Does not invoke the
//native global BEGIN_READ/READ_STRING machinery or parse native STL objects.
Result<ProfileInfo> InspectProfile(std::string_view bytes);

//Audit the native serializer/Open/Write/Flush/Close sequence without taking
//ownership of GameUI allocations or filesystem handles. Opening is authorized
//only after a complete, non-overflowed serialization with the expected identity.
class NativeWriteAudit
{
public:
    Status Serialized(std::string_view bytes, std::string_view rawUtf8, size_t capacity, bool overflow);
    Status Opening();
    Status Opened(bool success);
    Status Written(int requested, int actual);
    Status Closed(bool flushed);
    Status Finish() const;

private:
    Status   Fail(Error next);
    Error    error  = Error::None;
    size_t   length = 0;
    unsigned opens = 0, writes = 0, closes = 0;
};

enum class FileState
{
    Missing,
    Present,
    Unreadable,
    Invalid
};

struct ObservedProfile
{
    std::string filename; //A basename from this pack, never a raw player name.
    FileState   state = FileState::Missing;
    ProfileInfo profile;
    Error       error    = Error::None;
    uint64_t    fileTime = 0;
};

struct Diagnostic
{
    Error       error;
    std::string path;
};

struct ProfileEntry
{
    std::string rawUtf8;
    std::string path;
    uint64_t    fileTime;
};

class ProfileIndex
{
public:
    explicit ProfileIndex(std::string_view saveDirectory);
    //Call only after a complete directory scan and independent inspection.
    //Invalid/unreadable observations remain indexed to prevent legacy fallback.
    Status Add(ObservedProfile observed);
    Status FinishScan();
    void   MarkIncomplete(Error error, std::string_view path);

    Result<std::string>               ResolveRead(const Identity& identity) const;
    Status                            CheckWrite(const Identity& identity) const;
    Result<std::set<std::string>>     PlanDelete(const Identity& identity) const;
    Result<std::vector<ProfileEntry>> Enumerate() const;
    const std::vector<Diagnostic>&    Diagnostics() const noexcept { return diagnostics; }

private:
    struct Record
    {
        ObservedProfile observed;
        bool            canonical = false;
    };

    Error                   CheckIdentity(const Identity& identity) const;
    const Record*           Find(std::string_view path) const;
    Result<std::string>     Resolve(std::string_view rawUtf8, std::string_view canonicalPath) const;
    std::string             directory;
    std::vector<Record>     records;
    std::vector<Diagnostic> diagnostics;
    Error                   indexError   = Error::None;
    bool                    scanComplete = false;
};

//An absent canonical cfg schedules no exec. Legacy cfg is never an input.
Result<std::string> ConfigCommand(const Identity& identity, FileState canonicalState);

enum class Operation
{
    Create,
    Save,
    Load,
    Query,
    Delete,
    StartMatch,
    Enumerate
};

struct OperationContext
{
    Operation   operation;
    Identity    identity;
    std::string resolvedProfileReadPath;
    Error       error = Error::None;
};

class OperationScope
{
public:
    explicit OperationScope(OperationContext context);
    ~OperationScope();
    OperationScope(const OperationScope&)                     = delete;
    OperationScope&          operator=(const OperationScope&) = delete;
    static OperationContext* Current() noexcept;
    OperationContext&        Context() noexcept { return context; }

private:
    OperationContext                      context;
    OperationContext*                     previous;
    static thread_local OperationContext* current;
};

using PanelHandle = uint32_t;

struct SelectedIdentity
{
    std::string rawUtf8;
    std::string saveDirectory;
};

//Keys must be IVGui lifecycle handles, never object addresses. The validator
//must use HandleToPanel; all reads also check the owning frame is still alive.
class IdentitySidecars
{
public:
    explicit IdentitySidecars(std::function<bool(PanelHandle)> isLive);
    Status                                           BeginPopulate(PanelHandle frame, std::string_view saveDirectory);
    Result<std::string>                              AddButton(PanelHandle frame, PanelHandle button, std::string_view rawUtf8);
    Result<std::string>                              ButtonName(PanelHandle button) const;
    Result<std::string>                              TokenForName(PanelHandle frame, std::string_view rawUtf8) const;
    Status                                           SelectName(PanelHandle frame, std::string_view rawUtf8);
    std::vector<std::pair<PanelHandle, std::string>> Buttons(PanelHandle frame) const;
    Status                                           Select(PanelHandle frame, std::string_view token);
    Result<std::string>                              Selected(PanelHandle frame) const;
    Result<std::string>                              Confirm(PanelHandle frame, Operation operation, std::string_view createName = {});
    Result<SelectedIdentity>                         ConsumeConfirmation(PanelHandle frame, std::string_view token, Operation operation);
    void                                             CancelConfirmation(PanelHandle frame);
    void                                             Forget(PanelHandle panel);
    void                                             Clear();

private:
    struct Binding
    {
        PanelHandle button;
        std::string rawUtf8;
    };
    struct Frame
    {
        std::string                    directory;
        std::string                    selected;
        PanelHandle                    selectedButton = 0;
        std::map<std::string, Binding> tokens;
        std::string                    confirmation;
        std::string                    confirmedName;
        Operation                      confirmedOperation = Operation::Query;
    };
    struct Button
    {
        PanelHandle frame;
        std::string rawUtf8;
    };

    Frame*                           GetFrame(PanelHandle frame);
    const Frame*                     GetFrame(PanelHandle frame) const;
    Result<std::string>              NewToken();
    std::function<bool(PanelHandle)> isLive;
    std::map<PanelHandle, Frame>     frames;
    std::map<PanelHandle, Button>    buttons;
    uint64_t                         sequence = 0;
};
} //namespace CareerProfileStorage
