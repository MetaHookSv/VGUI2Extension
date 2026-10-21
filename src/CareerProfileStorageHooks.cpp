#include <metahook.h>
#include <vgui/IClientPanel.h>
#include <vgui/IPanel.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/QueryBox.h>
#include <KeyValues.h>
#include <intrin.h>

#include <array>
#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include "plugins.h"
#include "privatefuncs.h"
#include "exportfuncs.h"
#include "CareerProfileStorage.h"
#include "CareerProfileStorageHooks.h"
#include "CareerPlayerName.h"

namespace
{
using namespace CareerProfileStorage;
using StorageError = CareerProfileStorage::Error;

using VoidFn      = void(__fastcall*)(void*, int);
using NameFn      = void(__fastcall*)(void*, int, const char*);
using SaveFn      = bool(__fastcall*)(void*, int, bool);
using LoadFn      = bool(__fastcall*)(void*, int, int);
using QueryFn     = void*(__fastcall*)(void*, int, const char*);
using PopulateFn  = int(__fastcall*)(void*, int);
using SerializeFn = int(__fastcall*)(void*, int, void*, unsigned char*, int, bool);
using OpenFn      = FileHandle_t(__fastcall*)(void*, int, const char*, const char*, const char*);
using CloseFn     = void(__fastcall*)(void*, int, FileHandle_t);
using WriteFn     = int(__fastcall*)(void*, int, const void*, int, FileHandle_t);
using CreateDirFn = void(__fastcall*)(void*, int, const char*, const char*);
using CvarSetFn   = void(__cdecl*)(const char*, const char*);
using FreePanelFn = void(__fastcall*)(vgui::IVGui*, int, vgui::VPANEL);
using CopyFn      = char*(__cdecl*)(char*, const char*, size_t);

enum Entry
{
    Create,
    Save,
    Load,
    Query,
    Delete,
    StartMatch,
    Populate,
    ButtonName,
    UpdateSelection,
    OnCommand,
    NewCommand,
    Serialize,
    EntryCount
};

constexpr std::array<const char*, EntryCount> EntryNames = {
    "CCareerGame::CreateBlankProfile()",
    "CCareerGame::SaveProfile(bool)",
    "CCareerGame::LoadProfile(CareerDifficultyType)",
    "CCareerGame::GetProfileForName(char const*)",
    "CCareerGame::DeleteProfile(char const*)",
    "CCareerGame::SendMatchCommand()",
    "CCareerProfileFrame::PopulateProfiles()",
    "CProfileButton::SetProfileName(char const*)",
    "CCareerProfileFrame::UpdateSelectedProfile(char const*)",
    "CCareerProfileFrame::OnCommand(char const*)",
    "CCareerNewProfileBox::OnCommand(char const*)",
    "CCareerGame::CreateProfileBuffer(CCareerProfileData*, unsigned char*, int, bool)",
};

struct Patch
{
    const char*             group;
    uint32_t                count;
    uint32_t                abi;
    uint32_t                strategy;
    uint32_t                capacity;
    std::array<void*, 2>    sites{};
    std::array<uint32_t, 2> lengths{};
};

enum PatchIndex
{
    CreatePath,
    SavePath,
    LoadPath,
    QueryPath,
    DeletePath,
    WriteConfig,
    ExecConfig,
    SelectCommand,
    SelectedCopy,
    ResumeName,
    DoubleClick,
    EnumRead,
    PatchCount
};

std::array<Patch, PatchCount> patches = {{
    {"CCareerGame_CreateBlankProfile_profile_path_format_callsite", 1, 1, 0, 0},
    {"CCareerGame_SaveProfile_profile_path_format_callsite", 1, 1, 0, 0},
    {"CCareerGame_LoadProfile_profile_path_format_callsite", 1, 1, 0, 0},
    {"CCareerGame_GetProfileForName_profile_path_format_callsite", 1, 1, 0, 0},
    {"CCareerGame_DeleteProfile_profile_path_format_callsite", 1, 1, 0, 0},
    {"CCareerGame_SaveProfile_config_command_format_callsite", 1, 2, 0, 0},
    {"CCareerGame_SendMatchCommand_config_command_format_callsite", 1, 2, 0, 0},
    {"CCareerProfileFrame_PopulateProfiles_select_command_format_callsite", 1, 2, 0, 0},
    {"CCareerProfileFrame_selected_name_copy_callsite", 2, 5, 0, 0},
    {"CCareerProfileFrame_ResumeProfile_name_set_callsite", 1, 11, 2, 0},
    {"CProfileButton_OnMouseDoublePressed_select_command_format_callsite", 1, 6, 0, 0},
    {"CCareerGame_BuildProfileList_ReadFile_callsite", 1, 7, 1, 0},
}};

std::array<void*, EntryCount>    entries{};
std::array<uint32_t, EntryCount> entrySizes{};
std::array<void*, EntryCount>    originals{};
std::vector<hook_t*>             hooks;

void** definitionsStorage = nullptr;
void** gameStorage        = nullptr;
void** filesystemStorage  = nullptr;
void*  filesystem         = nullptr;
bool   supported          = false;
bool   installed          = false;

uint32_t activeOffset       = 0;
uint32_t directoryOffset    = 0;
uint32_t writerOffset       = 0;
uint32_t overflowOffset     = 0;
uint32_t needAutosaveOffset = 0;
uint32_t entryOffset        = 0;
uint32_t textLengthOffset   = 0;
uint32_t textReadSlot       = 0;
uint32_t frameNameOffset    = 0;
uint32_t buttonNameOffset   = 0;
uint32_t buttonActiveOffset = 0;
uint32_t profileCapacity    = 0;
uint32_t enumCapacity       = 0;

OpenFn      originalOpen      = nullptr;
CloseFn     originalClose     = nullptr;
WriteFn     originalWrite     = nullptr;
CreateDirFn originalCreateDir = nullptr;
CvarSetFn   originalCvarSet   = nullptr;
FreePanelFn originalFreePanel = nullptr;
CopyFn      originalCopy      = nullptr;

thread_local unsigned inspectorDepth = 0;

template <typename T>
T& Member(void* self, uint32_t offset)
{
    return *reinterpret_cast<T*>(static_cast<unsigned char*>(self) + offset);
}

template <typename F>
auto WithFs(F&& f)
{
    return f(static_cast<IFileSystem*>(filesystem));
}

struct InspectorGuard
{
    InspectorGuard() { ++inspectorDepth; }
    ~InspectorGuard() { --inspectorDepth; }
};

vgui::VPANEL VPanel(void* object)
{
    return static_cast<vgui::IClientPanel*>(object)->GetVPanel();
}

PanelHandle Handle(void* object)
{
    return vgui::ivgui()->PanelToHandle(VPanel(object));
}

bool Live(PanelHandle handle)
{
    return vgui::ivgui() && vgui::ivgui()->HandleToPanel(handle) != 0;
}

IdentitySidecars sidecars(Live);

struct RuntimeScope
{
    OperationScope                    domain;
    RuntimeScope*                     previous;
    std::unique_ptr<ProfileIndex>     index;
    PanelHandle                       frame = 0;
    std::string                       pendingDirectory;
    std::string                       pendingPathId;
    std::set<std::string>             enumerationPaths;
    FileHandle_t                      handle = nullptr;
    NativeWriteAudit                  writeAudit;
    unsigned                          pathFormats     = 0;
    bool                              configRequested = false;
    static thread_local RuntimeScope* current;

    explicit RuntimeScope(Operation operation, Identity identity) :
        domain({operation, std::move(identity)}), previous(current)
    {
        current = this;
    }

    ~RuntimeScope() { current = previous; }

    OperationContext& Context() { return domain.Context(); }
};

thread_local RuntimeScope* RuntimeScope::current = nullptr;

void Fail(StorageError error) noexcept
{
    if (auto* context = OperationScope::Current())
    {
        if (context->error == StorageError::None)
            context->error = error;
    }
}

void Log(StorageError error, const char* path = "") noexcept
{
    if (gEngfuncs.Con_Printf)
        gEngfuncs.Con_Printf("[VGUI2Extension] Career: %s %s\n", DescribeError(error), path);
}

void ShowError(StorageError error, const std::string& path = {})
{
    Log(error, path.c_str());

    std::string message = std::string(DescribeError(error)) + (path.empty() ? "" : "\n" + path);
    int         count   = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, message.c_str(), -1, nullptr, 0);
    if (count > 0)
    {
        std::wstring wide(static_cast<size_t>(count), 0);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, message.c_str(), -1, wide.data(), count);
        (new vgui::MessageBox(L"Career profile", wide.c_str(), nullptr))->DoModal();
    }
}

template <typename R, typename F>
R Boundary(F&& f, R failure) noexcept
{
    try
    {
        return f();
    }
    catch (...)
    {
        Fail(StorageError::ReadFailed);
        Log(StorageError::ReadFailed);
        return failure;
    }
}

template <typename F>
void BoundaryVoid(F&& f) noexcept
{
    Boundary<int>(
        [&] {
            f();
            return 0;
        },
        0);
}

Result<std::string> RawName(const char* text)
{
    if (!text)
        return {{}, StorageError::EmptyName};

    const size_t length = strnlen_s(text, ReaderStringCapacity);
    const auto   valid  = ValidateName({text, length});
    return valid ? Result<std::string>{std::string(text, length)} : Result<std::string>{{}, valid.error};
}

Result<std::string> SaveDirectory()
{
    if (!definitionsStorage || !*definitionsStorage)
        return {{}, StorageError::InvalidDirectory};

    void* active = Member<void*>(*definitionsStorage, activeOffset);
    if (!active)
        return {{}, StorageError::InvalidDirectory};

    const char* directory = Member<const char*>(active, directoryOffset);
    if (!directory || strnlen_s(directory, MAX_PATH) >= MAX_PATH)
        return {{}, StorageError::InvalidDirectory};

    return {directory};
}

std::string PhysicalRoot()
{
    std::string                root = std::string(GetBaseDirectory()) + "/" + gEngfuncs.pfnGetGameDirectory();
    std::array<char, MAX_PATH> absolute{};
    const DWORD                length = GetFullPathNameA(root.c_str(), static_cast<DWORD>(absolute.size()), absolute.data(), nullptr);
    if (!length || length >= absolute.size())
        return {};
    return absolute.data();
}

std::vector<std::string> PhysicalRoots()
{
    std::vector<std::string> roots;
    const auto               writeRoot = PhysicalRoot();
    if (writeRoot.empty())
        return roots;
    roots.push_back(writeRoot);

    std::array<char, MAX_PATH> local{};
    if (!WithFs([&](auto* fs) { return fs->GetLocalPath("", local.data(), static_cast<int>(local.size())); }) ||
        strnlen_s(local.data(), local.size()) >= local.size() - 1)
        return {};

    std::string readRoot(local.data());
    while (!readRoot.empty() && (readRoot.back() == '/' || readRoot.back() == '\\'))
        readRoot.pop_back();
    if (readRoot.empty())
        return {};

    if (WindowsPathKey(readRoot) != WindowsPathKey(writeRoot))
        roots.push_back(std::move(readRoot));
    return roots;
}

Result<Identity> MakeIdentity(std::string_view raw)
{
    auto directory = SaveDirectory();
    if (!directory)
        return {{}, directory.error};

    size_t prefix = 0;
    for (const auto& root : PhysicalRoots())
        prefix = (std::max)(prefix, root.size());

    PathLimits limits{
        ReaderStringCapacity,
        profileCapacity,
        enumCapacity,
        patches[WriteConfig].capacity,
        patches[ExecConfig].capacity,
        MAX_PATH,
        prefix,
    };
    return BuildIdentity(raw, directory.value, limits);
}

FileState State(const std::string& path)
{
    InspectorGuard guard;
    if (WithFs([&](auto* fs) { return fs->FileExists(path.c_str()); }))
        return FileState::Present;

    std::array<char, MAX_PATH> local{};
    if (WithFs([&](auto* fs) { return fs->GetLocalPath(path.c_str(), local.data(), static_cast<int>(local.size())); }))
        return FileState::Unreadable;

    const auto roots = PhysicalRoots();
    if (roots.empty())
        return FileState::Unreadable;

    for (const auto& root : roots)
    {
        const auto full = root + "/" + path;
        if (full.size() >= MAX_PATH || GetFileAttributesA(full.c_str()) != INVALID_FILE_ATTRIBUTES)
            return FileState::Unreadable;

        const auto error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            return FileState::Unreadable;
    }
    return FileState::Missing;
}

ObservedProfile Inspect(const std::string& directory, const std::string& filename)
{
    const auto      path = directory + "/" + filename;
    ObservedProfile observed{filename, State(path)};
    if (observed.state != FileState::Present)
    {
        observed.error = StorageError::ReadFailed;
        return observed;
    }

    std::array<char, MAX_PATH> local{};
    if (!WithFs([&](auto* fs) { return fs->GetLocalPath(path.c_str(), local.data(), static_cast<int>(local.size())); }) ||
        strnlen_s(local.data(), local.size()) >= local.size() - 1)
    {
        observed.state = FileState::Unreadable;
        observed.error = StorageError::UnverifiedCapacity;
        return observed;
    }

    InspectorGuard guard;
    auto           file = WithFs([&](auto* fs) { return fs->Open(path.c_str(), "rb"); });
    if (!file)
    {
        observed.state = FileState::Unreadable;
        observed.error = StorageError::ReadFailed;
        return observed;
    }

    struct FileCloser
    {
        FileHandle_t file;

        ~FileCloser()
        {
            WithFs([&](auto* fs) { fs->Close(file); });
        }
    } close{file};

    const auto size = WithFs([&](auto* fs) { return fs->Size(file); });
    if (size > ProfileBufferCapacity)
    {
        observed.state = FileState::Invalid;
        observed.error = StorageError::CapacityExceeded;
        return observed;
    }

    std::string bytes(size, 0);
    const auto  read = WithFs([&](auto* fs) { return fs->Read(bytes.data(), static_cast<int>(size), file); });
    if (read != static_cast<int>(size) || !WithFs([&](auto* fs) { return fs->IsOk(file); }))
    {
        observed.state = FileState::Unreadable;
        observed.error = StorageError::ReadFailed;
        return observed;
    }

    auto info         = InspectProfile(bytes);
    observed.state    = info ? FileState::Present : FileState::Invalid;
    observed.profile  = std::move(info.value);
    observed.error    = info.error;
    observed.fileTime = static_cast<uint32_t>(WithFs([&](auto* fs) { return fs->GetFileTime(path.c_str()); }));
    return observed;
}

std::unique_ptr<ProfileIndex> Scan(const Identity& identity)
{
    InspectorGuard        guard;
    auto                  index = std::make_unique<ProfileIndex>(identity.saveDirectory);
    std::set<std::string> filenames;
    FileFindHandle_t      find    = FILESYSTEM_INVALID_FIND_HANDLE;
    const auto            pattern = identity.saveDirectory + "/CZProfile-*.czp";
    const char*           name    = WithFs([&](auto* fs) { return fs->FindFirst(pattern.c_str(), &find); });

    struct FindCloser
    {
        FileFindHandle_t find;

        ~FindCloser()
        {
            if (find != FILESYSTEM_INVALID_FIND_HANDLE)
                WithFs([&](auto* fs) { fs->FindClose(find); });
        }
    } close{find};

    auto observe = [&](const char* basename) {
        std::string filename(basename);
        //FindFirst can repeat a basename from localized/fallback search paths.
        //Open resolves the same winning virtual path for each occurrence.
        if (!filenames.insert(WindowsPathKey(filename)).second)
            return;
        if (filename.size() + identity.saveDirectory.size() + 1 >= enumCapacity)
        {
            index->MarkIncomplete(StorageError::CapacityExceeded, filename);
            return;
        }
        index->Add(Inspect(identity.saveDirectory, filename));
    };

    for (; name; name = WithFs([&](auto* fs) { return fs->FindNext(find); }))
    {
        observe(name);
    }

    //The virtual filesystem has no enumeration-error API. Check the physical
    //read/write directories as well so access denied cannot become an empty pack.
    const auto roots = PhysicalRoots();
    if (roots.empty())
        index->MarkIncomplete(StorageError::UnverifiedCapacity, identity.saveDirectory);

    for (const auto& root : roots)
    {
        const auto       fullPattern = root + "/" + pattern;
        WIN32_FIND_DATAA data{};
        const auto       physicalFind = FindFirstFileA(fullPattern.c_str(), &data);
        if (physicalFind == INVALID_HANDLE_VALUE)
        {
            const auto error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
                index->MarkIncomplete(StorageError::ReadFailed, root + "/" + identity.saveDirectory);
            continue;
        }

        struct PhysicalFindCloser
        {
            HANDLE handle;

            ~PhysicalFindCloser() { FindClose(handle); }
        } physicalClose{physicalFind};

        do
        {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                observe(data.cFileName);
        } while (FindNextFileA(physicalFind, &data));

        if (GetLastError() != ERROR_NO_MORE_FILES)
            index->MarkIncomplete(StorageError::ReadFailed, fullPattern);
    }

    //Always probe the canonical candidate directly, including when enumeration
    //could not expose it. FileExists=false is not sufficient proof of absence.
    for (const auto& filename : {std::string("CZProfile-") + identity.storageKey + ".czp",
                                 std::string("CZProfile-") + LegacyKey(identity.rawUtf8) + ".czp"})
    {
        if (!filenames.insert(WindowsPathKey(filename)).second)
            continue;

        const auto state = State(identity.saveDirectory + "/" + filename);
        if (state != FileState::Missing)
            index->Add(Inspect(identity.saveDirectory, filename));
    }

    index->FinishScan();
    return index;
}

bool Prepare(RuntimeScope& scope, bool writing, bool reading)
{
    scope.index          = Scan(scope.Context().identity);
    const auto& identity = scope.Context().identity;
    if (writing)
    {
        auto status = scope.index->CheckWrite(identity);
        if (!status)
        {
            Fail(status.error);
            return false;
        }
    }
    if (reading)
    {
        auto resolved = scope.index->ResolveRead(identity);
        if (!resolved)
        {
            Fail(resolved.error);
            return false;
        }
        scope.Context().resolvedProfileReadPath = std::move(resolved.value);
    }
    return true;
}

RuntimeScope* HandleOwner(void* self, FileHandle_t handle)
{
    if (self != filesystem)
        return nullptr;

    for (auto* scope = RuntimeScope::current; scope; scope = scope->previous)
    {
        if (scope->handle == handle && handle)
            return scope;
    }
    return nullptr;
}

FileHandle_t __fastcall Open(void* self, int dummy, const char* path, const char* options, const char* pathId)
{
    const void* caller = _ReturnAddress();
    return Boundary<FileHandle_t>(
        [&]() -> FileHandle_t {
            auto* scope = RuntimeScope::current;
            if (!inspectorDepth && self == filesystem &&
                caller == static_cast<unsigned char*>(patches[EnumRead].sites[0]) + patches[EnumRead].lengths[0])
            {
                if (!scope || scope->Context().operation != Operation::Enumerate || !path || !options || std::strcmp(options, "rb") ||
                    !scope->enumerationPaths.erase(WindowsPathKey(path)))
                    return nullptr;
            }

            const bool saving       = scope && (scope->Context().operation == Operation::Create || scope->Context().operation == Operation::Save);
            const auto owner        = saving && scope->Context().operation == Operation::Create ? Create : Save;
            const auto address      = reinterpret_cast<uintptr_t>(caller);
            const bool nativeWriter = saving && address >= reinterpret_cast<uintptr_t>(entries[owner]) &&
                address < reinterpret_cast<uintptr_t>(entries[owner]) + entrySizes[owner];
            if (!inspectorDepth && nativeWriter && self == filesystem && options && std::strchr(options, 'w'))
            {
                auto& context = scope->Context();
                if (context.error != StorageError::None || scope->pathFormats != 1 || !path || path != context.identity.profilePath)
                {
                    Fail(StorageError::InvalidIdentity);
                    return nullptr;
                }

                auto ready = scope->writeAudit.Opening();
                if (!ready)
                {
                    Fail(ready.error);
                    return nullptr;
                }

                auto latest = Scan(context.identity);
                auto status = latest->CheckWrite(context.identity);
                if (!status)
                {
                    Fail(status.error);
                    return nullptr;
                }

                if (!scope->pendingDirectory.empty())
                {
                    originalCreateDir(self, dummy, scope->pendingDirectory.c_str(),
                                      scope->pendingPathId.empty() ? nullptr : scope->pendingPathId.c_str());
                }

                auto       file   = originalOpen(self, dummy, path, options, pathId);
                const auto opened = scope->writeAudit.Opened(file != nullptr);
                if (!opened)
                    Fail(opened.error);
                scope->handle = file;
                return file;
            }
            return originalOpen(self, dummy, path, options, pathId);
        },
        nullptr);
}

int __fastcall Write(void* self, int dummy, const void* data, int count, FileHandle_t file)
{
    const auto result = originalWrite(self, dummy, data, count, file);
    if (!inspectorDepth)
    {
        if (auto* scope = HandleOwner(self, file))
        {
            const auto written = scope->writeAudit.Written(count, result);
            if (!written && scope->Context().error == StorageError::None)
                scope->Context().error = written.error;
        }
    }
    return result;
}

void __fastcall Close(void* self, int dummy, FileHandle_t file)
{
    auto* scope = !inspectorDepth ? HandleOwner(self, file) : nullptr;
    if (scope)
    {
        WithFs([&](auto* fs) { fs->Flush(file); });
        const auto closed = scope->writeAudit.Closed(WithFs([&](auto* fs) { return fs->IsOk(file); }));
        if (!closed && scope->Context().error == StorageError::None)
            scope->Context().error = closed.error;
        scope->handle = nullptr;
    }
    originalClose(self, dummy, file);
}

void __fastcall CareerCreateDirectory(void* self, int dummy, const char* path, const char* pathId)
{
    BoundaryVoid([&] {
        auto* scope = RuntimeScope::current;
        if (!inspectorDepth && self == filesystem && scope &&
            (scope->Context().operation == Operation::Create || scope->Context().operation == Operation::Save))
        {
            if (!path || WindowsPathKey(path) != WindowsPathKey(scope->Context().identity.saveDirectory))
            {
                Fail(StorageError::InvalidDirectory);
                return;
            }
            scope->pendingDirectory = scope->Context().identity.saveDirectory;
            scope->pendingPathId    = pathId ? pathId : "";
            return;
        }
        originalCreateDir(self, dummy, path, pathId);
    });
}

int __fastcall SerializeProfile(void* self, int dummy, void* profile, unsigned char* buffer, int capacity, bool fresh)
{
    return Boundary<int>(
        [&] {
            const int length = reinterpret_cast<SerializeFn>(originals[Serialize])(self, dummy, profile, buffer, capacity, fresh);
            auto*     scope  = RuntimeScope::current;
            if (!scope)
                return length;

            auto* writer = Member<void*>(self, writerOffset);
            if (!writer || length <= 0 || length > capacity)
            {
                Fail(StorageError::CapacityExceeded);
                return length;
            }

            const auto status = scope->writeAudit.Serialized({reinterpret_cast<const char*>(buffer), static_cast<size_t>(length)},
                                                             scope->Context().identity.rawUtf8,
                                                             static_cast<size_t>(capacity),
                                                             Member<unsigned char>(writer, overflowOffset) != 0);
            if (!status)
                Fail(status.error);
            return length;
        },
        0);
}

int Copy(char* destination, size_t capacity, std::string_view text)
{
    if (destination && capacity)
        destination[0] = 0;
    if (!destination || text.size() >= capacity)
    {
        Fail(StorageError::CapacityExceeded);
        return -1;
    }
    std::memcpy(destination, text.data(), text.size());
    destination[text.size()] = 0;
    return static_cast<int>(text.size());
}

template <PatchIndex Index, Operation Expected>
int __cdecl ProfilePath(char* destination, const char* format, const char* directory, const char*)
{
    if (destination)
        destination[0] = 0;
    return Boundary<int>(
        [&] {
            auto* scope = RuntimeScope::current;
            if (!scope || scope->Context().operation != Expected || !format || std::strcmp(format, "%s/CZProfile-%s.czp") ||
                !directory || WindowsPathKey(directory) != WindowsPathKey(scope->Context().identity.saveDirectory))
            {
                Fail(StorageError::InvalidIdentity);
                return -1;
            }

            ++scope->pathFormats;
            const auto& context = scope->Context();
            return Copy(destination, patches[Index].capacity,
                        Expected == Operation::Load || Expected == Operation::Query ? context.resolvedProfileReadPath : context.identity.profilePath);
        },
        -1);
}

template <PatchIndex Index, Operation Expected>
int __cdecl ConfigFormat(char* destination, size_t capacity, const char* format, const char*)
{
    if (destination && capacity)
        destination[0] = 0;
    return Boundary<int>(
        [&] {
            auto*       scope    = RuntimeScope::current;
            const char* expected = Expected == Operation::Save ? "writecfg Save/%s\n" : "wait\nexec Save/%s.cfg\n";
            if (!scope || scope->Context().operation != Expected || !format || std::strcmp(format, expected) ||
                capacity != patches[Index].capacity)
            {
                Fail(StorageError::InvalidIdentity);
                return -1;
            }

            auto& context = scope->Context();
            if (context.error != StorageError::None)
                return Copy(destination, capacity, "\n");

            if constexpr (Expected == Operation::Save)
            {
                const auto written = scope->writeAudit.Finish();
                if (!written)
                {
                    Fail(written.error);
                    return Copy(destination, capacity, "\n");
                }
                scope->configRequested = true;
                return Copy(destination, capacity, context.identity.writeConfigCommand);
            }
            else
            {
                auto command = ConfigCommand(context.identity, State(context.identity.configPath));
                if (!command)
                {
                    Fail(command.error);
                    return Copy(destination, capacity, "\n");
                }
                return Copy(destination, capacity, command.value.empty() ? std::string_view("wait\n") : std::string_view(command.value));
            }
        },
        -1);
}

void FixActive(PanelHandle frame, bool focus = false)
{
    const auto selected = sidecars.Selected(frame);
    for (const auto& button : sidecars.Buttons(frame))
    {
        const auto panel = vgui::ivgui()->HandleToPanel(button.first);
        if (auto* object = vgui::ipanel()->Client(panel))
        {
            const bool active                                 = selected && selected.value == button.second;
            Member<unsigned char>(object, buttonActiveOffset) = active;
            vgui::ipanel()->Repaint(panel);
            if (focus && active)
                vgui::ipanel()->RequestFocus(panel);
        }
    }
}

void __fastcall SetButtonName(void* self, int dummy, const char* raw)
{
    BoundaryVoid([&] {
        auto* scope = RuntimeScope::current;
        if (!scope || scope->Context().operation != Operation::Enumerate)
        {
            Fail(StorageError::InvalidIdentity);
            return;
        }

        auto name = RawName(raw);
        if (!name)
        {
            Fail(name.error);
            return;
        }

        auto token = sidecars.AddButton(scope->frame, Handle(self), name.value);
        if (!token)
        {
            Fail(token.error);
            return;
        }

        reinterpret_cast<NameFn>(originals[ButtonName])(self, dummy, raw);

        std::wstring wide;
        if (ConvertCareerPlayerNameToUnicode(raw, wide))
        {
            using SetTextFn = void(__fastcall*)(void*, int, const wchar_t*);
            auto method     = reinterpret_cast<SetTextFn>((*static_cast<void***>(self))[gPrivateFuncs.vfunc_index_Label_SetTextW]);
            method(self, 0, wide.c_str());
        }
    });
}

void __fastcall UpdateSelected(void* self, int dummy, const char* raw)
{
    BoundaryVoid([&] {
        const auto frame = Handle(self);
        auto       name  = raw == static_cast<char*>(self) + frameNameOffset ? sidecars.Selected(frame) : RawName(raw);
        if (!name)
        {
            if (raw && !*raw)
            {
                reinterpret_cast<NameFn>(originals[UpdateSelection])(self, dummy, raw);
                return;
            }
            Fail(name.error);
            return;
        }

        auto selected = sidecars.SelectName(frame, name.value);
        if (!selected)
        {
            Fail(selected.error);
            return;
        }

        reinterpret_cast<NameFn>(originals[UpdateSelection])(self, dummy, name.value.c_str());
        FixActive(frame);
    });
}

char* __cdecl SelectedNameCopy(char* destination, const char* source, size_t capacity)
{
    return Boundary<char*>(
        [&] {
            if (capacity != LegacyKeyCapacity || !destination || !source)
            {
                Fail(StorageError::CapacityExceeded);
                return destination;
            }

            void* frame = destination - frameNameOffset;
            if (source != destination && *source)
            {
                auto status = sidecars.SelectName(Handle(frame), source);
                if (!status)
                    Fail(status.error);
            }
            //Preserve the native bounded field and padding; full identity lives in sidecars.
            return destination == source ? destination : originalCopy(destination, source, capacity);
        },
        destination);
}

int __cdecl SelectionFormat(char* destination, size_t capacity, const char* format, const char* raw)
{
    if (destination && capacity)
        destination[0] = 0;
    return Boundary<int>(
        [&] {
            auto* scope = RuntimeScope::current;
            if (!scope || scope->Context().operation != Operation::Enumerate || capacity != patches[SelectCommand].capacity ||
                !format || std::strcmp(format, "SelectProfileAndFocus %s") || !raw)
            {
                Fail(StorageError::InvalidIdentity);
                return -1;
            }

            auto token = sidecars.TokenForName(scope->frame, raw);
            if (!token)
            {
                Fail(token.error);
                return -1;
            }
            return Copy(destination, capacity, "SelectProfileAndFocus " + token.value);
        },
        -1);
}

thread_local std::array<std::array<char, 64>, 4> doubleClickCommands{};
thread_local size_t                              doubleClickSlot = 0;

char* __cdecl DoubleClickFormat(const char* format, const char* field)
{
    static char empty[] = "";
    return Boundary<char*>(
        [&]() -> char* {
            if (!field || !format || std::strcmp(format, "SelectProfile %s"))
                return empty;

            void* button = const_cast<char*>(field) - buttonNameOffset;
            auto  name   = sidecars.ButtonName(Handle(button));
            if (!name)
                return empty;

            auto parent = vgui::ipanel()->GetParent(VPanel(button));
            while (parent)
            {
                auto token = sidecars.TokenForName(vgui::ivgui()->PanelToHandle(parent), name.value);
                if (token)
                {
                    auto& command = doubleClickCommands[doubleClickSlot++ % doubleClickCommands.size()];
                    Copy(command.data(), command.size(), "SelectProfile " + token.value);
                    return command.data();
                }
                parent = vgui::ipanel()->GetParent(parent);
            }
            return empty;
        },
        empty);
}

void __cdecl SetCvar(const char* name, const char* value)
{
    const void* caller = _ReturnAddress();
    BoundaryVoid([&] {
        auto* scope = RuntimeScope::current;
        if (caller == static_cast<unsigned char*>(patches[ResumeName].sites[0]) + patches[ResumeName].lengths[0] &&
            name && !std::strcmp(name, "name"))
        {
            while (scope && !scope->frame)
                scope = scope->previous;
            if (!scope || !scope->frame)
            {
                Fail(StorageError::StaleSelection);
                return;
            }

            auto selected = sidecars.Selected(scope->frame);
            if (!selected)
            {
                Fail(selected.error);
                return;
            }
            originalCvarSet(name, selected.value.c_str());
            return;
        }
        originalCvarSet(name, value);
    });
}

bool WroteProfile(RuntimeScope& scope)
{
    const auto status = scope.writeAudit.Finish();
    if (scope.Context().error == StorageError::None)
        scope.Context().error = scope.handle ? StorageError::CoverageMismatch : status.error;
    return scope.Context().error == StorageError::None;
}

void __fastcall CreateProfile(void* self, int dummy)
{
    BoundaryVoid([&] {
        auto* scope = RuntimeScope::current;
        //Only the UI handler grants overwrite authority, after its exact-name
        //confirmation and ownership preflight. Original branches never run.
        if (!scope || scope->Context().operation != Operation::Create || !scope->frame || !scope->index)
        {
            ShowError(StorageError::InvalidIdentity);
            return;
        }
        reinterpret_cast<VoidFn>(originals[Create])(self, dummy);
    });
}

bool __fastcall SaveProfile(void* self, int dummy, bool updateConfig)
{
    return Boundary<bool>(
        [&] {
            //Verified hl-10210 guard returns true before allocating, formatting or I/O.
            if (!Member<unsigned char>(self, needAutosaveOffset))
                return reinterpret_cast<SaveFn>(originals[Save])(self, dummy, updateConfig);

            auto raw      = RawName(gEngfuncs.pfnGetCvarString("name"));
            auto identity = raw ? MakeIdentity(raw.value) : Result<Identity>{{}, raw.error};
            if (!identity)
            {
                ShowError(identity.error);
                return false;
            }

            RuntimeScope scope(Operation::Save, std::move(identity.value));
            if (!Prepare(scope, true, false))
            {
                ShowError(scope.Context().error, scope.Context().identity.profilePath);
                return false;
            }

            const bool result = reinterpret_cast<SaveFn>(originals[Save])(self, dummy, updateConfig);
            if (!result || !WroteProfile(scope))
            {
                ShowError(scope.Context().error == StorageError::None ? StorageError::ReadFailed : scope.Context().error,
                          scope.Context().identity.profilePath);
                return false;
            }

            gEngfuncs.Con_Printf("[VGUI2Extension] Career CZP written: %s%s\n", scope.Context().identity.profilePath.c_str(),
                                 scope.configRequested ? "; CFG write requested (queued)" : "");
            return true;
        },
        false);
}

bool __fastcall LoadProfile(void* self, int dummy, int difficulty)
{
    return Boundary<bool>(
        [&] {
            auto raw      = RawName(gEngfuncs.pfnGetCvarString("name"));
            auto identity = raw ? MakeIdentity(raw.value) : Result<Identity>{{}, raw.error};
            if (!identity)
            {
                ShowError(identity.error);
                return false;
            }

            RuntimeScope scope(Operation::Load, std::move(identity.value));
            if (!Prepare(scope, false, true))
            {
                ShowError(scope.Context().error, scope.Context().identity.profilePath);
                return false;
            }

            const bool result = reinterpret_cast<LoadFn>(originals[Load])(self, dummy, difficulty);
            return result && scope.Context().error == StorageError::None && scope.pathFormats == 1;
        },
        false);
}

void* __fastcall QueryProfile(void* self, int dummy, const char* legacy)
{
    return Boundary<void*>(
        [&]() -> void* {
            const auto* caller = OperationScope::Current();
            if (!caller || caller->operation != Operation::Query || caller->error != StorageError::None)
            {
                Fail(StorageError::InvalidIdentity);
                return nullptr;
            }

            RuntimeScope scope(Operation::Query, caller->identity);
            if (!Prepare(scope, false, true))
            {
                if (scope.Context().error != StorageError::NotFound)
                    ShowError(scope.Context().error, scope.Context().identity.profilePath);
                return nullptr;
            }

            auto result = reinterpret_cast<QueryFn>(originals[Query])(self, dummy, legacy);
            return scope.Context().error == StorageError::None && scope.pathFormats == 1 ? result : nullptr;
        },
        nullptr);
}

Status DeleteOwned(const Identity& identity)
{
    auto index = Scan(identity);
    auto plan  = index->PlanDelete(identity);
    if (!plan)
    {
        ShowError(plan.error, identity.profilePath);
        return {plan.error};
    }

    Status status;
    for (const auto& path : plan.value)
    {
        WithFs([&](auto* fs) { fs->RemoveFile(path.c_str()); });
        if (State(path) != FileState::Missing)
        {
            status.error = StorageError::DeleteFailed;
            ShowError(status.error, path);
        }
    }
    return status;
}

void __fastcall DeleteProfile(void*, int, const char* raw)
{
    BoundaryVoid([&] {
        auto name     = RawName(raw);
        auto identity = name ? MakeIdentity(name.value) : Result<Identity>{{}, name.error};
        if (!identity)
        {
            ShowError(identity.error);
            return;
        }

        RuntimeScope scope(Operation::Delete, std::move(identity.value));
        const auto   status = DeleteOwned(scope.Context().identity);
        if (!status)
            Fail(status.error);
    });
}

void __fastcall SendMatch(void* self, int dummy)
{
    BoundaryVoid([&] {
        auto raw      = RawName(gEngfuncs.pfnGetCvarString("name"));
        auto identity = raw ? MakeIdentity(raw.value) : Result<Identity>{{}, raw.error};
        if (!identity)
        {
            ShowError(identity.error);
            return;
        }

        RuntimeScope scope(Operation::StartMatch, std::move(identity.value));
        auto         command = ConfigCommand(scope.Context().identity, State(scope.Context().identity.configPath));
        if (!command)
        {
            ShowError(command.error, scope.Context().identity.configPath);
            return;
        }

        if (command.value.empty())
            gEngfuncs.Con_Printf("[VGUI2Extension] Career canonical CFG absent; shared legacy CFG was not imported. The next normal save requests a new CFG.\n");
        reinterpret_cast<VoidFn>(originals[StartMatch])(self, dummy);
    });
}

int __fastcall PopulateProfiles(void* self, int dummy)
{
    return Boundary<int>(
        [&] {
            const auto frame = Handle(self);
            sidecars.Forget(frame);
            //An arbitrary valid name builds the pack paths; enumeration does not
            //derive disk identity from the current or truncated selected name.
            auto identity = MakeIdentity("a");
            if (!identity)
            {
                ShowError(identity.error);
                return 0;
            }

            RuntimeScope scope(Operation::Enumerate, std::move(identity.value));
            scope.frame = frame;
            auto status = sidecars.BeginPopulate(scope.frame, scope.Context().identity.saveDirectory);
            if (!status)
            {
                ShowError(status.error);
                return 0;
            }

            scope.index = Scan(scope.Context().identity);
            auto list   = scope.index->Enumerate();
            if (!list)
            {
                ShowError(list.error);
                return 0;
            }

            for (const auto& diagnostic : scope.index->Diagnostics())
                Log(diagnostic.error, diagnostic.path.c_str());
            for (const auto& item : list.value)
                scope.enumerationPaths.insert(WindowsPathKey(item.path));

            const auto count = reinterpret_cast<PopulateFn>(originals[Populate])(self, dummy);
            //Native enumeration need not expose every fallback-root candidate in
            //the ownership index. Every button it actually builds must be captured.
            if (count < 0 || sidecars.Buttons(scope.frame).size() != static_cast<size_t>(count))
                Fail(StorageError::CoverageMismatch);
            if (scope.Context().error != StorageError::None)
            {
                sidecars.Forget(scope.frame);
                ShowError(scope.Context().error);
                return 0;
            }

            FixActive(scope.frame);
            return count;
        },
        0);
}

void NativeCommand(void* frame, const char* command)
{
    reinterpret_cast<NameFn>(originals[OnCommand])(frame, 0, command);
}

void __fastcall ProfileCommand(void* self, int, const char* command);

void ConfirmOperation(void* frame, Operation operation, const Identity& identity)
{
    auto token = sidecars.Confirm(Handle(frame), operation, operation == Operation::Create ? identity.rawUtf8 : std::string_view{});
    if (!token)
    {
        ShowError(token.error);
        return;
    }

    const auto command = std::string(operation == Operation::Create ? "CareerStorageCreate " : "CareerStorageDelete ") + token.value;
    auto*      box     = new vgui::QueryBox(L"Career profile",
                                            operation == Operation::Create ? L"Overwrite the existing profile with this complete player name?" : L"Delete this profile and its verified legacy copies?",
                                            nullptr);
    box->SetParent(VPanel(frame));
    box->AddActionSignalTarget(static_cast<vgui::Panel*>(frame));
    box->SetOKCommand(new KeyValues("Command", "command", command.c_str()));
    box->SetCancelCommand(new KeyValues("Command", "command", "CareerStorageCancel"));
    box->DoModal();
}

void CreateFromUi(void* self, const Identity& identity, bool confirmed)
{
    RuntimeScope scope(Operation::Create, identity);
    scope.frame = Handle(self);
    if (!Prepare(scope, true, false))
    {
        ShowError(scope.Context().error, identity.profilePath);
        return;
    }

    auto existing = scope.index->ResolveRead(identity);
    if (!existing && existing.error != StorageError::NotFound)
    {
        ShowError(existing.error, identity.profilePath);
        return;
    }
    if (existing && !confirmed)
    {
        ConfirmOperation(self, Operation::Create, identity);
        return;
    }
    if (!gameStorage || !*gameStorage)
    {
        ShowError(StorageError::InvalidIdentity);
        return;
    }

    struct NameRollback
    {
        std::string previous  = gEngfuncs.pfnGetCvarString("name");
        bool        committed = false;

        ~NameRollback()
        {
            if (!committed)
                originalCvarSet("name", previous.c_str());
        }
    } nameRollback;

    originalCvarSet("name", identity.rawUtf8.c_str());
    if (identity.rawUtf8 != gEngfuncs.pfnGetCvarString("name"))
    {
        ShowError(StorageError::CapacityExceeded);
        return;
    }

    CreateProfile(*gameStorage, 0);
    if (!WroteProfile(scope))
    {
        ShowError(scope.Context().error, identity.profilePath);
        return;
    }
    nameRollback.committed = true;

    if (PopulateProfiles(self, 0) <= 0)
        return;
    UpdateSelected(self, 0, identity.rawUtf8.c_str());

    auto selected = sidecars.Selected(scope.frame);
    if (scope.Context().error != StorageError::None || !selected || selected.value != identity.rawUtf8)
    {
        ShowError(scope.Context().error != StorageError::None ? scope.Context().error : StorageError::StaleSelection);
        return;
    }
    ProfileCommand(self, 0, "ResumeProfile");
}

void DeleteFromUi(void* self, const Identity& identity, bool confirmed)
{
    auto index = Scan(identity);
    auto plan  = index->PlanDelete(identity);
    if (!plan)
    {
        ShowError(plan.error, identity.profilePath);
        return;
    }
    if (!confirmed)
    {
        ConfirmOperation(self, Operation::Delete, identity);
        return;
    }

    RuntimeScope scope(Operation::Delete, identity);
    if (!DeleteOwned(identity))
        Fail(StorageError::DeleteFailed);
    PopulateProfiles(self, 0);
}

void __fastcall ProfileCommand(void* self, int, const char* command)
{
    BoundaryVoid([&] {
        if (!command)
            return;

        const std::string_view cmd(command);
        const auto             frame = Handle(self);
        if (cmd == "CareerStorageCancel")
        {
            sidecars.CancelConfirmation(frame);
            return;
        }

        const bool createConfirmation = cmd.substr(0, 20) == "CareerStorageCreate ";
        const bool deleteConfirmation = cmd.substr(0, 20) == "CareerStorageDelete ";
        if (createConfirmation || deleteConfirmation)
        {
            const auto operation = createConfirmation ? Operation::Create : Operation::Delete;
            auto       selected  = sidecars.ConsumeConfirmation(frame, cmd.substr(20), operation);
            if (!selected)
            {
                ShowError(selected.error);
                return;
            }

            auto identity = MakeIdentity(selected.value.rawUtf8);
            if (!identity || identity.value.saveDirectory != selected.value.saveDirectory)
            {
                ShowError(StorageError::StaleConfirmation);
                return;
            }

            if (createConfirmation)
            {
                CreateFromUi(self, identity.value, true);
            }
            else
            {
                DeleteFromUi(self, identity.value, true);
            }
            return;
        }

        if (cmd.substr(0, 17) == "CareerStorageNew ")
        {
            auto raw      = DecodeKey(cmd.substr(17));
            auto identity = raw ? MakeIdentity(raw.value) : Result<Identity>{{}, raw.error};
            if (!identity)
            {
                ShowError(identity.error);
                return;
            }
            CreateFromUi(self, identity.value, false);
            return;
        }

        if (!_stricmp(command, "CreateProfile"))
        {
            auto raw      = RawName(gEngfuncs.pfnGetCvarString("name"));
            auto identity = raw ? MakeIdentity(raw.value) : Result<Identity>{{}, raw.error};
            if (!identity)
            {
                ShowError(identity.error);
                return;
            }
            CreateFromUi(self, identity.value, false);
            return;
        }

        if (!_stricmp(command, "ReallyCreateProfile") || !_stricmp(command, "ReallyDeleteProfile"))
        {
            ShowError(StorageError::StaleConfirmation);
            return;
        }

        if (cmd.substr(0, 14) == "SelectProfile " || cmd.substr(0, 22) == "SelectProfileAndFocus ")
        {
            const bool focus  = cmd.substr(0, 22) == "SelectProfileAndFocus ";
            auto       status = sidecars.Select(frame, cmd.substr(focus ? 22 : 14));
            if (!status)
            {
                ShowError(status.error);
                return;
            }

            auto name = sidecars.Selected(frame);
            UpdateSelected(self, 0, name.value.c_str());
            FixActive(frame, focus);
            return;
        }

        const bool resume = !_stricmp(command, "ResumeProfile");
        const bool choose = cmd.substr(0, 16) == "ChooseDifficulty";
        const bool remove = !_stricmp(command, "DeleteProfile");
        if (resume || choose || remove)
        {
            auto raw = sidecars.Selected(frame);
            if (!raw)
            {
                if (resume)
                {
                    NativeCommand(self, "NewProfile");
                }
                else
                {
                    ShowError(raw.error);
                }
                return;
            }

            auto identity = MakeIdentity(raw.value);
            if (!identity)
            {
                ShowError(identity.error);
                return;
            }

            if (remove)
            {
                DeleteFromUi(self, identity.value, false);
                return;
            }

            RuntimeScope scope(Operation::Query, std::move(identity.value));
            scope.frame = frame;
            if (!Prepare(scope, false, true))
            {
                ShowError(scope.Context().error, scope.Context().identity.profilePath);
                return;
            }

            originalCvarSet("name", scope.Context().identity.rawUtf8.c_str());
            NativeCommand(self, command);
            FixActive(frame);
            return;
        }

        NativeCommand(self, command);
    });
}

void __fastcall NewProfileCommand(void* self, int dummy, const char* command)
{
    BoundaryVoid([&] {
        if (!command)
            return;
        if (_stricmp(command, "Ok"))
        {
            reinterpret_cast<NameFn>(originals[NewCommand])(self, dummy, command);
            return;
        }

        auto* entry = Member<void*>(self, entryOffset);
        if (!entry)
        {
            ShowError(StorageError::EmptyName);
            return;
        }

        const auto length = Member<int>(entry, textLengthOffset);
        if (length < 1 || static_cast<size_t>(length) >= ReaderStringCapacity)
        {
            ShowError(length ? StorageError::CapacityExceeded : StorageError::EmptyName);
            return;
        }

        std::wstring wide(static_cast<size_t>(length) + 1, 0);
        using GetTextFn = void(__fastcall*)(void*, int, wchar_t*, int);
        //The verified hl-10210 implementation bounds wchar_t elements, despite
        //the public SDK header naming this parameter bufLenInBytes.
        reinterpret_cast<GetTextFn>((*static_cast<void***>(entry))[textReadSlot])(entry, 0, wide.data(), length + 1);
        if (std::find(wide.begin(), wide.begin() + length, L'\0') != wide.begin() + length)
        {
            ShowError(StorageError::UnsafeName);
            return;
        }

        const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), length, nullptr, 0, nullptr, nullptr);
        if (bytes <= 0)
        {
            ShowError(StorageError::InvalidUtf8);
            return;
        }

        std::string raw(static_cast<size_t>(bytes), 0);
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), length, raw.data(), bytes, nullptr, nullptr);
        auto identity = MakeIdentity(raw);
        if (!identity)
        {
            ShowError(identity.error);
            return;
        }

        //Skip the lossy 64-byte apply routine. The original still forwards the
        //command and closes its own native box with its native allocator.
        //Carry the complete identity in the action signal without changing name
        //until creation is confirmed and ready to write.
        const auto createCommand = std::string("CareerStorageNew ") + identity.value.storageKey;
        reinterpret_cast<NameFn>(originals[NewCommand])(self, dummy, createCommand.c_str());
    });
}

void __fastcall FreePanel(vgui::IVGui* self, int dummy, vgui::VPANEL panel)
{
    BoundaryVoid([&] { sidecars.Forget(self->PanelToHandle(panel)); });
    originalFreePanel(self, dummy, panel);
}
} //namespace

namespace
{
uint32_t Scalar(const std::string& name)
{
    uint32_t value = 0;
    if (g_pMetaHookAPI->QueryGameSymbolScalar(g_GameUIDllInfo.ImageBase, name.c_str(), &value) != MH_GAMESYMBOL_OK)
        throw std::runtime_error("Missing Career scalar: " + name);
    return value;
}

void Require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void RecordHook(hook_t* hook)
{
    Require(hook != nullptr, "Career hook installation failed");
    hooks.push_back(hook);
}

void PatchBranch(PatchIndex index, unsigned number, void* target, void** original = nullptr)
{
    RecordHook(g_pMetaHookAPI->InlinePatchRedirectBranch(patches[index].sites[number], target, original));
}
} //namespace

void CareerStorage_FillAddress()
{
    supported = false;
    //The existing CZero flag also includes Deleted Scenes, which has no
    //reviewed Career storage flow. Enable only the czero campaign mode.
    if (!g_bIsCZero || g_bIsCZDS)
        return;

    if (g_pMetaHookAPI->IsGameSymbolAvailable(g_GameUIDllInfo.ImageBase, "CCareerGame_u8hex_storage_contract_version") != MH_GAMESYMBOL_OK)
    {
        if (g_dwEngineBuildnum == 10210)
            Sys_Error("Missing required GameUI Career storage contract for build 10210.");
        return;
    }

    try
    {
        Require(Scalar("CCareerGame_u8hex_storage_contract_version") == 1 &&
                    Scalar("CareerStorage_runtime_contract_version") == 1,
                "Unsupported Career storage contract");

        for (size_t i = 0; i < entries.size(); ++i)
        {
            const auto kind = i == OnCommand || i == NewCommand ? MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION : MH_GAMESYMBOL_KIND_FUNCTION;
            entries[i]      = GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", EntryNames[i], kind);

            mh_gamesymbol_t symbol{};
            symbol.cbSize = sizeof(symbol);
            Require(g_pMetaHookAPI->QueryGameSymbol(g_GameUIDllInfo.ImageBase, EntryNames[i], &symbol) == MH_GAMESYMBOL_OK && symbol.symbolSize,
                    "Missing Career entry bounds");
            entrySizes[i] = symbol.symbolSize;
        }

        for (size_t i = 0; i < patches.size(); ++i)
        {
            auto& patch = patches[i];
            Require(Scalar(std::string(patch.group) + "_count") == patch.count, "Career callsite coverage differs");
            if (i <= SelectCommand)
                patch.capacity = Scalar(std::string(patch.group) + "_destination_capacity");

            for (unsigned n = 0; n < patch.count; ++n)
            {
                const auto name = std::string(patch.group) + "_" + std::to_string(n);
                Require(Scalar(name + "_abi") == patch.abi && Scalar(name + "_hook_strategy") == patch.strategy,
                        "Unsupported Career adapter ABI or strategy");
                patch.lengths[n]  = Scalar(name + "_instruction_length");
                patch.sites[n]    = GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", name.c_str(), MH_GAMESYMBOL_KIND_PATCH);
                const auto* bytes = static_cast<unsigned char*>(patch.sites[n]);
                if (patch.strategy == 0)
                {
                    Require((patch.lengths[n] == 5 && bytes[0] == 0xe8) ||
                                (patch.lengths[n] == 6 && bytes[0] == 0xff && bytes[1] == 0x15),
                            "Career branch bytes differ from the verified adapter");
                }
                else if (patch.strategy == 1)
                {
                    Require(patch.lengths[n] == 3 && bytes[0] == 0xff && bytes[1] == 0x52 && bytes[2] == 0x28,
                            "Career enumeration Open instruction differs");
                }
                else
                {
                    Require(patch.lengths[n] == 2 && bytes[0] == 0xff && bytes[1] == 0xd0,
                            "Career Resume Cvar_Set instruction differs");
                }
            }
        }

        profileCapacity = patches[CreatePath].capacity;
        for (size_t i = CreatePath; i <= DeletePath; ++i)
            Require(patches[i].capacity == profileCapacity, "Career path capacities differ");

        enumCapacity = Scalar("CCareerGame_BuildProfileList_enumerated_path_capacity");
        Require(Scalar("CCareerGame_profile_buffer_capacity") == ProfileBufferCapacity &&
                    Scalar("CCareerGame_legacy_key_capacity") == LegacyKeyCapacity &&
                    Scalar("CProfileButton_name_capacity") == LegacyKeyCapacity &&
                    Scalar("CCareerProfileFrame_selected_name_capacity") == LegacyKeyCapacity &&
                    Scalar("CCareerNewProfileBox_native_input_capacity") == 64,
                "Unsupported Career input or serializer capacity");

        auto member = [&](const char* name) {
            return GamedataResolveStructMember(g_GameUIDllInfo.ImageBase, "gameui", name);
        };
        activeOffset       = member("CCareerDefinitionManager.m_activeCareer");
        directoryOffset    = member("CCareerDefinition.m_saveDir");
        writerOffset       = member("CCareerGame.m_profileWriter");
        overflowOffset     = member("BufferWriter.m_overflow");
        needAutosaveOffset = member("CCareerGame.m_needAutosave");
        entryOffset        = member("CCareerNewProfileBox.m_pNameEntry");
        textLengthOffset   = member("vgui2::TextEntry.m_textLength");
        frameNameOffset    = member("CCareerProfileFrame.m_name");
        buttonNameOffset   = member("CProfileButton.m_name");
        buttonActiveOffset = member("CProfileButton.m_active");
        textReadSlot       = GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::TextEntry::GetText(wchar_t*, int)");

        auto global = [&](const char* name) {
            return static_cast<void**>(GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", name, MH_GAMESYMBOL_KIND_GLOBAL));
        };
        definitionsStorage = global("CareerStorage_Definitions");
        gameStorage        = global("CareerStorage_Game");
        filesystemStorage  = global("CareerStorage_FileSystem");
        supported          = true;
    }
    catch (const std::exception& error)
    {
        Sys_Error("Could not prepare Career storage hooks: %s", error.what());
    }
}

void CareerStorage_InstallHooks()
{
    if (!supported || installed)
        return;

    try
    {
        Require(filesystemStorage && *filesystemStorage, "GameUI filesystem was not initialized");
        filesystem = *filesystemStorage;
        Require(g_pFileSystem && filesystem == g_pFileSystem, "GameUI filesystem does not match the verified legacy public interface");
        Require(vgui::ivgui() && gEngfuncs.Cvar_Set, "Career public interfaces were not initialized");
        hooks.reserve(32);

        //This GameUI binary calls legacy Write at 0x58. Even on an HL25 engine
        //it must receive the legacy public compatibility interface, not the
        //engine interface with the inserted GetFileChangeTime slot.
        auto** table = *static_cast<void***>(filesystem);
        RecordHook(g_pMetaHookAPI->VFTHookEx(table, 10, reinterpret_cast<void*>(Open), reinterpret_cast<void**>(&originalOpen)));
        RecordHook(g_pMetaHookAPI->VFTHookEx(table, 11, reinterpret_cast<void*>(Close), reinterpret_cast<void**>(&originalClose)));
        RecordHook(g_pMetaHookAPI->VFTHookEx(table, 22, reinterpret_cast<void*>(Write), reinterpret_cast<void**>(&originalWrite)));
        RecordHook(g_pMetaHookAPI->VFTHookEx(table, 7, reinterpret_cast<void*>(CareerCreateDirectory), reinterpret_cast<void**>(&originalCreateDir)));
        RecordHook(g_pMetaHookAPI->VFTHookEx(*reinterpret_cast<void***>(vgui::ivgui()), 9,
                                             reinterpret_cast<void*>(FreePanel), reinterpret_cast<void**>(&originalFreePanel)));
        RecordHook(g_pMetaHookAPI->InlineHook(reinterpret_cast<void*>(gEngfuncs.Cvar_Set),
                                              reinterpret_cast<void*>(SetCvar), reinterpret_cast<void**>(&originalCvarSet)));

        const std::array<void*, EntryCount> replacements = {
            reinterpret_cast<void*>(CreateProfile),
            reinterpret_cast<void*>(SaveProfile),
            reinterpret_cast<void*>(LoadProfile),
            reinterpret_cast<void*>(QueryProfile),
            reinterpret_cast<void*>(DeleteProfile),
            reinterpret_cast<void*>(SendMatch),
            reinterpret_cast<void*>(PopulateProfiles),
            reinterpret_cast<void*>(SetButtonName),
            reinterpret_cast<void*>(UpdateSelected),
            reinterpret_cast<void*>(ProfileCommand),
            reinterpret_cast<void*>(NewProfileCommand),
            reinterpret_cast<void*>(SerializeProfile),
        };
        for (size_t i = 0; i < entries.size(); ++i)
            RecordHook(g_pMetaHookAPI->InlineHook(entries[i], replacements[i], &originals[i]));

        PatchBranch(CreatePath, 0, reinterpret_cast<void*>(ProfilePath<CreatePath, Operation::Create>));
        PatchBranch(SavePath, 0, reinterpret_cast<void*>(ProfilePath<SavePath, Operation::Save>));
        PatchBranch(LoadPath, 0, reinterpret_cast<void*>(ProfilePath<LoadPath, Operation::Load>));
        PatchBranch(QueryPath, 0, reinterpret_cast<void*>(ProfilePath<QueryPath, Operation::Query>));
        //DeletePath was validated above. The replacement entry never calls it.
        PatchBranch(WriteConfig, 0, reinterpret_cast<void*>(ConfigFormat<WriteConfig, Operation::Save>));
        PatchBranch(ExecConfig, 0, reinterpret_cast<void*>(ConfigFormat<ExecConfig, Operation::StartMatch>));
        PatchBranch(SelectCommand, 0, reinterpret_cast<void*>(SelectionFormat));

        void* firstCopy = nullptr;
        PatchBranch(SelectedCopy, 0, reinterpret_cast<void*>(SelectedNameCopy), &firstCopy);
        originalCopy = reinterpret_cast<decltype(originalCopy)>(firstCopy);

        void* secondCopy = nullptr;
        PatchBranch(SelectedCopy, 1, reinterpret_cast<void*>(SelectedNameCopy), &secondCopy);
        Require(firstCopy == secondCopy, "Career selected-name copies have different CRT targets");

        PatchBranch(DoubleClick, 0, reinterpret_cast<void*>(DoubleClickFormat));
        installed = true;
    }
    catch (const std::exception& error)
    {
        CareerStorage_UninstallHooks();
        Sys_Error("Could not install complete Career storage hooks: %s", error.what());
    }
}

void CareerStorage_UninstallHooks()
{
    installed = false;
    for (auto it = hooks.rbegin(); it != hooks.rend(); ++it)
        g_pMetaHookAPI->UnHook(*it);
    hooks.clear();
    sidecars.Clear();
    originals.fill(nullptr);
    originalOpen      = nullptr;
    originalClose     = nullptr;
    originalWrite     = nullptr;
    originalCreateDir = nullptr;
    originalCvarSet   = nullptr;
    originalFreePanel = nullptr;
    originalCopy      = nullptr;
    filesystem        = nullptr;
}

void* CareerStorage_ConstructDifficulty(CareerDifficultyCtor original, void* self, int dummy, const char* name, void* parent)
{
    if (!installed)
        return original(self, dummy, name, parent);

    return Boundary<void*>(
        [&] {
            auto         raw      = RawName(gEngfuncs.pfnGetCvarString("name"));
            auto         identity = raw ? MakeIdentity(raw.value) : Result<Identity>{{}, raw.error};
            RuntimeScope scope(Operation::Query, std::move(identity.value));
            scope.Context().error = identity.error;
            return original(self, dummy, name, parent);
        },
        nullptr);
}
