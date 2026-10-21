#include "../src/CareerProfileStorage.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>

using namespace CareerProfileStorage;

namespace
{
const std::string chineseTest = "\xe6\xb5\x8b\xe8\xaf\x95test";
const std::string chineseName = "\xe4\xb8\xad\xe6\x96\x87test";

PathLimits Limits()
{
    //Published hl-10210 capacities. The physical prefix is supplied by the caller.
    return {2047, 260, 260, 260, 1024, 260, 64};
}

void Put32(std::string& bytes, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<char>((value >> shift) & 255));
}

void Seal(std::string& bytes)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < bytes.size(); ++i)
        sum += static_cast<uint32_t>(i + 1) * static_cast<unsigned char>(bytes[i]);
    Put32(bytes, sum);
}

std::string Profile(std::string_view raw, uint32_t version = 4, bool populated = true)
{
    std::string bytes("CZP\0", 4);
    Put32(bytes, version);
    bytes.append(raw);
    bytes.push_back(0);
    Put32(bytes, 17);
    if (version >= 4)
        bytes.push_back(2);
    bytes.append(4, 0);
    for (int difficulty = 0; difficulty < 4; ++difficulty)
    {
        const bool present = populated && (difficulty == 0 || difficulty == 3);
        bytes.push_back(present ? 1 : 0);
        if (!present)
            continue;
        bytes.push_back(2);
        bytes.push_back(2);
        bytes.append("Nick\0Josh\0", 10);
        bytes.push_back(3);
        for (int map = 0; map < 3; ++map)
        {
            bytes.append("de_dust2\0", 9);
            bytes.push_back(1);
        }
        bytes.append("de_dust2\0", 9);
        if (version >= 3)
            bytes.push_back(1);
    }
    if (version != 1)
    {
        Put32(bytes, 256);
        bytes.append(256 * 4, 0);
    }
    Seal(bytes);
    return bytes;
}

ObservedProfile Observe(std::string filename, std::string_view name, uint64_t time = 1)
{
    auto inspection = InspectProfile(Profile(name));
    assert(inspection);
    return {std::move(filename), FileState::Present, std::move(inspection.value), Error::None, time};
}

void EncodingAndPaths()
{
    assert("u8hex-e6b58be8af9574657374" == EncodeKey(chineseTest).value);
    assert("u8hex-f09f9880" == EncodeKey("\xf0\x9f\x98\x80").value);
    assert("u8hex-41" == EncodeKey("A").value);
    assert("u8hex-61" == EncodeKey("a").value);
    assert("u8hex-75386865782d3631" == EncodeKey("u8hex-61").value);
    assert("u8hex-204120" == EncodeKey(" A ").value);
    assert(EncodeKey("A / B%hs"));
    for (const auto& raw : {chineseTest, chineseName, std::string("______test"), std::string("\xf0\x9f\x98\x80")})
    {
        const auto key = EncodeKey(raw);
        assert(key);
        assert(raw == DecodeKey(key.value).value);
        assert(key.value.find_first_not_of("u8hex-0123456789abcdef") == std::string::npos);
    }
    assert(EncodeKey(chineseTest).value != EncodeKey(chineseName).value);
    assert("______test" == LegacyKey(chineseTest));
    assert("______test" == LegacyKey(chineseName));
    assert(std::string(30, 'A') + "_" == LegacyKey(std::string(30, 'A') + "\xe4\xb8\xadZ"));
    assert(std::string(31, 'A') == LegacyKey(std::string(40, 'A')));

    for (const auto& raw : {std::string(), std::string("a\0b", 3), std::string("a\nb"), std::string("a\tb"),
                            std::string("a\"b"), std::string("a;b"), std::string("a\\b"), std::string("a\x7f")})
        assert(!EncodeKey(raw));
    for (const auto& raw : {"\xc0\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\x80", "\xe4\xb8", "\xf5\x80\x80\x80"})
        assert(Error::InvalidUtf8 == EncodeKey(raw).error);
    for (const auto& key : {"u8hex-", "u8hex-6", "u8hex-6A", "U8HEX-61", "u8hex-zz", "u8hex-c080", "u8hex-00", "a"})
        assert(!DecodeKey(key));

    auto identity = BuildIdentity(chineseTest, "MissionPacks/Test/Save", Limits());
    assert(identity);
    assert("MissionPacks/Test/Save/CZProfile-u8hex-e6b58be8af9574657374.czp" == identity.value.profilePath);
    assert("Save/u8hex-e6b58be8af9574657374.cfg" == identity.value.configPath);
    assert("writecfg Save/u8hex-e6b58be8af9574657374\n" == identity.value.writeConfigCommand);
    assert("wait\nexec Save/u8hex-e6b58be8af9574657374.cfg\n" == identity.value.execConfigCommand);
    assert("MissionPacks/Test/Save" == BuildIdentity("a", "MissionPacks\\Test\\Save", Limits()).value.saveDirectory);
    for (const auto& directory : {"", "/Save", "C:/Save", "../Save", "Pack/../Save", "Pack//Save", "Pack/./Save", "Save/", "Save.", "Save ", "Save?", "Pack\nSave"})
        assert(Error::InvalidDirectory == BuildIdentity("a", directory, Limits()).error);

    //All boundaries include the terminating NUL; no partial identity escapes.
    auto boundary = BuildIdentity(std::string(31, 'A') + "B", "Save", Limits());
    assert(boundary);
    assert(boundary.value.storageKey != BuildIdentity(std::string(31, 'A') + "C", "Save", Limits()).value.storageKey);
    auto limits                = Limits();
    auto base                  = BuildIdentity("a", "Save", limits).value;
    limits.profilePathCapacity = base.profilePath.size() + 1;
    assert(BuildIdentity("a", "Save", limits));
    --limits.profilePathCapacity;
    auto failed = BuildIdentity("a", "Save", limits);
    assert(Error::CapacityExceeded == failed.error);
    assert(failed.value.profilePath.empty());
    limits                            = Limits();
    limits.physicalGameDirectoryBytes = 0;
    assert(Error::UnverifiedCapacity == BuildIdentity("a", "Save", limits).error);
    limits                            = Limits();
    limits.physicalGameDirectoryBytes = (std::numeric_limits<size_t>::max)();
    assert(Error::CapacityExceeded == BuildIdentity("a", "Save", limits).error);
    limits                 = Limits();
    limits.rawNameCapacity = 32;
    assert(BuildIdentity(std::string(31, 'A'), "Save", limits));
    assert(Error::CapacityExceeded == BuildIdentity(std::string(32, 'A'), "Save", limits).error);
    for (auto member : {&PathLimits::profilePathCapacity, &PathLimits::enumeratedPathCapacity,
                        &PathLimits::writeCommandCapacity, &PathLimits::execCommandCapacity, &PathLimits::systemPathCapacity})
    {
        limits         = Limits();
        limits.*member = 1;
        assert(Error::CapacityExceeded == BuildIdentity("a", "Save", limits).error);
        limits.*member = 0;
        assert(Error::UnverifiedCapacity == BuildIdentity("a", "Save", limits).error);
    }
}

void Inspector()
{
    for (uint32_t version = 1; version <= 4; ++version)
    {
        const auto bytes  = Profile(chineseTest, version);
        const auto parsed = InspectProfile(bytes);
        assert(parsed);
        assert(version == parsed.value.version);
        assert(chineseTest == parsed.value.rawUtf8);
        for (size_t length = 0; length < bytes.size(); ++length)
            assert(!InspectProfile(std::string_view(bytes).substr(0, length)));
        auto bad = bytes;
        bad.back() ^= 1;
        assert(Error::ChecksumMismatch == InspectProfile(bad).error);
    }
    assert(Error::UnsupportedVersion == InspectProfile(Profile("a", 5)).error);
    assert(Error::UnsupportedVersion == InspectProfile(Profile("a", 0)).error);
    auto bytes   = Profile("a", 4, false);
    auto altered = bytes.substr(0, bytes.size() - 4);
    altered.push_back(0);
    Seal(altered);
    assert(Error::MalformedProfile == InspectProfile(altered).error);
    altered    = bytes.substr(0, bytes.size() - 4);
    altered[0] = 'X';
    Seal(altered);
    assert(Error::MalformedProfile == InspectProfile(altered).error);
    assert(Error::InvalidUtf8 == InspectProfile(Profile("\xe4\xb8")).error);
    assert(Error::UnsafeName == InspectProfile(Profile("a;b")).error);
    assert(Error::CapacityExceeded == InspectProfile(std::string(16385, 'X')).error);

    //Malformed counts retain a valid checksum, so structural bounds are exercised.
    altered = Profile("a", 4, true);
    altered.resize(altered.size() - 4);
    const size_t firstDifficulty = 4 + 4 + 2 + 4 + 1 + 4;
    altered[firstDifficulty + 2] = 6;
    Seal(altered);
    assert(Error::MalformedProfile == InspectProfile(altered).error);
    altered = Profile("a", 4, true);
    altered.resize(altered.size() - 4);
    altered[firstDifficulty + 3 + 10] = 2;
    Seal(altered);
    assert(Error::MalformedProfile == InspectProfile(altered).error);
    altered                 = bytes.substr(0, bytes.size() - 4);
    const size_t tutorCount = firstDifficulty + 4;
    std::fill(altered.begin() + tutorCount, altered.begin() + tutorCount + 4, static_cast<char>(0xff));
    Seal(altered);
    assert(Error::MalformedProfile == InspectProfile(altered).error);
    altered = Profile("a", 4, true);
    altered.resize(altered.size() - 4);
    altered[firstDifficulty + 3] = static_cast<char>(0xff);
    Seal(altered);
    assert(Error::MalformedProfile == InspectProfile(altered).error);
}

void OwnershipAndRouting()
{
    auto         identity = BuildIdentity(chineseTest, "Pack/Save", Limits()).value;
    ProfileIndex index(identity.saveDirectory);
    assert(Error::IncompleteIndex == index.CheckWrite(identity).error);
    assert(index.Add(Observe("CZProfile-______test.czp", chineseTest)));
    assert(index.FinishScan());
    assert("Pack/Save/CZProfile-______test.czp" == index.ResolveRead(identity).value);
    assert(index.CheckWrite(identity));
    assert(index.Add(Observe("CZProfile-" + identity.storageKey + ".czp", chineseTest, 2)));
    assert(index.FinishScan());
    assert(identity.profilePath == index.ResolveRead(identity).value);
    assert(1 == index.Enumerate().value.size());
    assert(2 == index.PlanDelete(identity).value.size());
    assert(1 == index.PlanDelete(identity).value.count(identity.profilePath));
    assert(0 == index.PlanDelete(identity).value.count(identity.configPath));
    assert(Error::WrongMissionPack == index.ResolveRead(BuildIdentity(chineseTest, "Other/Save", Limits()).value).error);
    assert(Error::NotFound == index.ResolveRead(BuildIdentity(chineseName, "Pack/Save", Limits()).value).error);

    //An old ASCII name can occupy another player's canonical namespace.
    auto         a = BuildIdentity("a", "Save", Limits()).value;
    ProfileIndex collision("Save");
    assert(collision.Add(Observe("CZProfile-u8hex-61.czp", "u8hex-61")));
    assert(collision.FinishScan());
    assert(Error::IdentityConflict == collision.ResolveRead(a).error);
    assert(Error::IdentityConflict == collision.CheckWrite(a).error);
    assert(Error::IdentityConflict == collision.PlanDelete(a).error);
    auto oldAscii = BuildIdentity("u8hex-61", "Save", Limits()).value;
    assert("Save/CZProfile-u8hex-61.czp" == collision.ResolveRead(oldAscii).value);

    for (auto state : {FileState::Unreadable, FileState::Invalid})
    {
        ProfileIndex bad("Save");
        assert(bad.Add(Observe("CZProfile-a.czp", "a")));
        assert(!bad.Add({"CZProfile-u8hex-61.czp", state, {}, Error::MalformedProfile, 2}));
        assert(bad.FinishScan());
        assert(!bad.ResolveRead(a));
        assert(!bad.CheckWrite(a));
        assert(!bad.PlanDelete(a));
        assert(bad.Enumerate().value.empty());
        assert(!bad.Diagnostics().empty());
        ProfileIndex badLegacy("Save");
        assert(badLegacy.Add(Observe("CZProfile-u8hex-61.czp", "a")));
        assert(!badLegacy.Add({"CZProfile-a.czp", state, {}, Error::ReadFailed, 2}));
        assert(badLegacy.FinishScan());
        assert(badLegacy.ResolveRead(a));
        assert(badLegacy.CheckWrite(a));
        assert(!badLegacy.PlanDelete(a));
    }
    ProfileIndex invalid("Save");
    assert(!invalid.Add(Observe("CZProfile-u8hex-6A.czp", "j")));
    assert(!invalid.Add(Observe("CZProfile-wrong.czp", "a")));
    assert(!invalid.Add(Observe("../CZProfile-a.czp", "a")));
    ProfileIndex caseDuplicate("Save");
    assert(caseDuplicate.Add(Observe("CZProfile-A.czp", "A")));
    assert(!caseDuplicate.Add(Observe("czprofile-a.CZP", "a")));
    assert(Error::AmbiguousFiles == caseDuplicate.CheckWrite(a).error);
    ProfileIndex incomplete("Save");
    incomplete.MarkIncomplete(Error::ReadFailed, "Save");
    assert(Error::IncompleteIndex == incomplete.CheckWrite(a).error);
    assert(Error::IncompleteIndex == incomplete.ResolveRead(a).error);
    assert(Error::IncompleteIndex == incomplete.PlanDelete(a).error);

    //Enumeration preserves native time order and input order for ties.
    ProfileIndex ordered("Save");
    assert(ordered.Add(Observe("CZProfile-z.czp", "z", 5)));
    assert(ordered.Add(Observe("CZProfile-A.czp", "A", 5)));
    assert(ordered.Add(Observe("CZProfile-u8hex-61.czp", "a", 6)));
    assert(ordered.FinishScan());
    auto list = ordered.Enumerate();
    assert(list);
    assert(3 == list.value.size());
    assert("a" == list.value[0].rawUtf8);
    assert("z" == list.value[1].rawUtf8);
    assert("A" == list.value[2].rawUtf8);
    assert(ConfigCommand(a, FileState::Missing).value.empty());
    assert(a.execConfigCommand == ConfigCommand(a, FileState::Present).value);
    assert(!ConfigCommand(a, FileState::Unreadable));
}

void NativeWriteFailures()
{
    const auto       bytes  = Profile("a", 4);
    const int        length = static_cast<int>(bytes.size());
    NativeWriteAudit write;
    assert(write.Serialized(bytes, "a", ProfileBufferCapacity, false));
    assert(write.Opening());
    assert(write.Opened(true));
    assert(write.Written(length, length));
    assert(write.Closed(true));
    assert(write.Finish());
    assert(Error::CoverageMismatch == write.Opening().error);

    for (auto failure : {Error::CapacityExceeded, Error::InvalidIdentity, Error::OpenFailed,
                         Error::ShortWrite, Error::FlushFailed, Error::CoverageMismatch})
    {
        NativeWriteAudit audit;
        if (failure == Error::CapacityExceeded)
            assert(!audit.Serialized(bytes, "a", ProfileBufferCapacity, true));
        else if (failure == Error::InvalidIdentity)
            assert(!audit.Serialized(bytes, "A", ProfileBufferCapacity, false));
        else
        {
            assert(audit.Serialized(bytes, "a", ProfileBufferCapacity, false));
            assert(audit.Opening());
            audit.Opened(failure != Error::OpenFailed);
            audit.Written(length, failure == Error::ShortWrite ? length - 1 : length);
            if (failure != Error::CoverageMismatch)
                audit.Closed(failure != Error::FlushFailed);
        }
        assert(failure == audit.Finish().error);
    }
    NativeWriteAudit unopened;
    assert(!unopened.Opening());
    NativeWriteAudit corrupt;
    assert(!corrupt.Serialized(bytes.substr(0, bytes.size() - 1), "a", ProfileBufferCapacity, false));
    assert(!corrupt.Opening());
    NativeWriteAudit wrongCapacity;
    assert(!wrongCapacity.Serialized(bytes, "a", ProfileBufferCapacity - 1, false));
}

void ScopesAndUi()
{
    auto first  = BuildIdentity(std::string(31, 'A') + "B", "Save", Limits()).value;
    auto second = BuildIdentity(std::string(31, 'A') + "C", "Save", Limits()).value;
    assert(nullptr == OperationScope::Current());
    {
        OperationScope outer({Operation::Save, first});
        assert(first.rawUtf8 == OperationScope::Current()->identity.rawUtf8);
        try
        {
            OperationScope inner({Operation::Query, second});
            assert(second.rawUtf8 == OperationScope::Current()->identity.rawUtf8);
            throw std::runtime_error("scope");
        }
        catch (const std::runtime_error&)
        {
            assert(first.rawUtf8 == OperationScope::Current()->identity.rawUtf8);
        }
    }
    assert(nullptr == OperationScope::Current());

    std::set<PanelHandle> live{11, 12, 13, 21};
    IdentitySidecars      ui([&](PanelHandle handle) { return live.count(handle) != 0; });
    assert(ui.BeginPopulate(11, "Save"));
    assert(!ui.AddButton(11, 11, first.rawUtf8));
    auto token1 = ui.AddButton(11, 12, first.rawUtf8);
    auto token2 = ui.AddButton(11, 13, second.rawUtf8);
    assert(token1 && token2);
    assert(token1.value != token2.value);
    assert(token1.value.size() < 32);
    assert(token1.value.find(first.rawUtf8) == std::string::npos);
    assert(ui.Select(11, token1.value));
    assert(first.rawUtf8 == ui.Selected(11).value);
    auto confirmation = ui.Confirm(11, Operation::Delete);
    assert(confirmation);
    assert(ui.Select(11, token2.value));
    assert(!ui.ConsumeConfirmation(11, confirmation.value, Operation::Delete));
    confirmation = ui.Confirm(11, Operation::Delete);
    assert(!ui.ConsumeConfirmation(21, confirmation.value, Operation::Delete));
    assert(!ui.ConsumeConfirmation(11, confirmation.value, Operation::Create));
    assert(ui.ConsumeConfirmation(11, confirmation.value, Operation::Delete));
    assert(!ui.ConsumeConfirmation(11, confirmation.value, Operation::Delete));
    confirmation = ui.Confirm(11, Operation::Delete);
    assert(ui.BeginPopulate(11, "Other/Save"));
    assert(!ui.Select(11, token1.value));
    assert(!ui.ConsumeConfirmation(11, confirmation.value, Operation::Delete));
    assert(!ui.ButtonName(12));
    assert(ui.AddButton(11, 12, first.rawUtf8));
    live.erase(12);
    assert(!ui.ButtonName(12));
    //The replacement native panel has a different lifecycle handle, even if its address is reused.
    live.insert(22);
    assert(!ui.ButtonName(22));
    live.erase(11);
    assert(!ui.Selected(11));
    assert(!ui.ButtonName(13));
    ui.Clear();
    assert(!ui.Selected(21));
    assert(ui.BeginPopulate(21, "Save"));
    auto finalToken = ui.AddButton(21, 13, "a");
    assert(finalToken && ui.Select(21, finalToken.value));
    assert(!ui.BeginPopulate(21, "../Save"));
    assert(!ui.Select(21, finalToken.value));
    assert("pack/save/czprofile-a.czp" == WindowsPathKey("Pack\\Save\\CZProfile-A.CZP"));
}
} //namespace

int main(int argc, char** argv)
{
    EncodingAndPaths();
    Inspector();
    OwnershipAndRouting();
    NativeWriteFailures();
    ScopesAndUi();
    //Optional read-only check of a real CZP; no dependency on an installed game in CTest.
    for (int i = 1; i < argc; ++i)
    {
        std::ifstream file(argv[i], std::ios::binary);
        assert(file);
        const std::string bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        assert(InspectProfile(bytes));
    }
}
