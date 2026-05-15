#include "FileManager.h"
#include "ProjectVersion.h"

#include "Routing.h"
#include "core/utils/StringBuilder.h"
#include "core/fs/FileSystem.h"
#include "core/fs/FileWriter.h"
#include "core/fs/FileReader.h"

#include "os/os.h"
#include "os/LockGuard.h"

#include <algorithm>

#include <cstring>

uint32_t FileManager::_volumeState = 0;
uint32_t FileManager::_nextVolumeStateCheckTicks = 0;

std::array<FileManager::CachedSlotInfo, 4> FileManager::_cachedSlotInfos;
uint32_t FileManager::_cachedSlotInfoTicket = 0;

FileManager::TaskExecuteCallback FileManager::_taskExecuteCallback;
FileManager::TaskResultCallback FileManager::_taskResultCallback;
FileManager::TaskResultCallback FileManager::_taskResultCallbackPending;
fs::Error FileManager::_taskResultPendingValue = fs::OK;
volatile uint32_t FileManager::_taskPending;
volatile uint32_t FileManager::_taskExecuting;
volatile uint32_t FileManager::_taskResultPending;
static os::Mutex fileTaskMutex;

struct FileTypeInfo {
    const char *dir;
    const char *ext;
};

FileTypeInfo fileTypeInfos[] = {
    { "PROJECTS", "PRO" },
    { "SCALES", "SCA" },
    {"SEQS", "NSQ"},
    {"SEQS", "CSQ"},
    {"SEQS", "LSQ"},
    {"SEQS", "ASQ"}
};

static void slotPath(StringBuilder &str, FileType type, int slot) {
    const auto &info = fileTypeInfos[int(type)];
    str("%s/%03d.%s", info.dir, slot + 1, info.ext);
}

void FileManager::init() {
    _volumeState = 0;
    _nextVolumeStateCheckTicks = 0;
    _taskExecuteCallback = nullptr;
    _taskResultCallback = nullptr;
    _taskResultCallbackPending = nullptr;
    _taskResultPendingValue = fs::OK;
    _taskPending = 0;
    _taskExecuting = 0;
    _taskResultPending = 0;
}

bool FileManager::volumeAvailable() {
    return _volumeState & Available;
}

bool FileManager::volumeMounted() {
    return _volumeState & Mounted;
}

fs::Error FileManager::format() {
    invalidateAllSlots();
    return fs::volume().format();
}

fs::Error FileManager::writeProject(Project &project, int slot) {
    return writeFile(FileType::Project, slot, [&] (const char *path) {
        auto result = writeProject(project, path);
        if (result == fs::OK) {
            project.setSlot(slot);
            writeLastProject(slot);
        }
        return result;
    });
}

fs::Error FileManager::readProject(Project &project, int slot) {
    return readFile(FileType::Project, slot, [&] (const char *path) {
        auto result = readProject(project, path);
        if (result == fs::OK) {
            project.setSlot(slot);
            writeLastProject(slot);
        }
        return result;
    });
}

fs::Error FileManager::readLastProject(Project &project) {
    int slot;

    auto result = readLastProject(slot);

    if (result == fs::OK && slot >= 0) {
        result = readProject(project, slot);
        project.setAutoLoaded(true);
    }

    return result;
}

fs::Error FileManager::writeUserScale(const UserScale &userScale, int slot) {
    return writeFile(FileType::UserScale, slot, [&] (const char *path) {
        return writeUserScale(userScale, path);
    });
}

fs::Error FileManager::readUserScale(UserScale &userScale, int slot) {
    return readFile(FileType::UserScale, slot, [&] (const char *path) {
        return readUserScale(userScale, path);
    });
}


fs::Error FileManager::writeNoteSequence(const NoteSequence &noteSequence, int slot) {
    return writeFile(FileType::NoteSequence, slot, [&] (const char *path) {
        return writeNoteSequence(noteSequence, path);
    });
}

fs::Error FileManager::readNoteSequence(NoteSequence &noteSequence, int slot) {
    return readFile(FileType::NoteSequence, slot, [&] (const char *path) {
        return readNoteSequence(noteSequence, path);
    });
}

fs::Error FileManager::writeCurveSequence(const CurveSequence &curveSequence, int slot) {
    return writeFile(FileType::CurveSequence, slot, [&] (const char *path) {
        return writeCurveSequence(curveSequence, path);
    });
}

fs::Error FileManager::readCurveSequence(CurveSequence &curveSequence, int slot) {
    return readFile(FileType::CurveSequence, slot, [&] (const char *path) {
        return readCurveSequence(curveSequence, path);
    });
}

fs::Error FileManager::writeLogicSequence(const LogicSequence &noteSequence, int slot) {
    return writeFile(FileType::LogicSequence, slot, [&] (const char *path) {
        return writeLogicSequence(noteSequence, path);
    });
}

fs::Error FileManager::readLogicSequence(LogicSequence &noteSequence, int slot) {
    return readFile(FileType::LogicSequence, slot, [&] (const char *path) {
        return readLogicSequence(noteSequence, path);
    });
}

fs::Error FileManager::writeArpSequence(const ArpSequence &noteSequence, int slot) {
    return writeFile(FileType::ArpSequence, slot, [&] (const char *path) {
        return writeArpSequence(noteSequence, path);
    });
}

fs::Error FileManager::readArpSequence(ArpSequence &noteSequence, int slot) {
    return readFile(FileType::ArpSequence, slot, [&] (const char *path) {
        return readArpSequence(noteSequence, path);
    });
}

fs::Error FileManager::writeProject(const Project &project, const char *path) {
    fs::FileWriter fileWriter(path);
    if (fileWriter.error() != fs::OK) {
        return fileWriter.error();
    }

    FileHeader header(FileType::Project, 0, project.name());
    fileWriter.write(&header, sizeof(header));

    VersionedSerializedWriter writer(
        [&fileWriter] (const void *data, size_t len) { fileWriter.write(data, len); },
        ProjectVersion::Latest
    );

    project.write(writer);

    return fileWriter.finish();
}

fs::Error FileManager::readProject(Project &project, const char *path) {
    fs::FileReader fileReader(path);
    if (fileReader.error() != fs::OK) {
        return fileReader.error();
    }

    FileHeader header;
    fileReader.read(&header, sizeof(header));

    VersionedSerializedReader reader(
        [&fileReader] (void *data, size_t len) { fileReader.read(data, len); },
        ProjectVersion::Latest
    );

    bool success = project.read(reader);

    auto error = fileReader.finish();
    if (error == fs::OK && !success) {
        error = fs::INVALID_CHECKSUM;
    }

    return error;
}

fs::Error FileManager::writeUserScale(const UserScale &userScale, const char *path) {
    fs::FileWriter fileWriter(path);
    if (fileWriter.error() != fs::OK) {
        return fileWriter.error();
    }

    FileHeader header(FileType::UserScale, 0, userScale.name());
    fileWriter.write(&header, sizeof(header));

    VersionedSerializedWriter writer(
        [&fileWriter] (const void *data, size_t len) { fileWriter.write(data, len); },
        ProjectVersion::Latest
    );

    userScale.write(writer);

    return fileWriter.finish();
}

fs::Error FileManager::readUserScale(UserScale &userScale, const char *path) {
    fs::FileReader fileReader(path);
    if (fileReader.error() != fs::OK) {
        return fileReader.error();
    }

    FileHeader header;
    fileReader.read(&header, sizeof(header));

    VersionedSerializedReader reader(
        [&fileReader] (void *data, size_t len) { fileReader.read(data, len); },
        ProjectVersion::Latest
    );

    bool success = userScale.read(reader);

    auto error = fileReader.finish();
    if (error == fs::OK && !success) {
        error = fs::INVALID_CHECKSUM;
    }

    return error;
}

fs::Error FileManager::writeNoteSequence(const NoteSequence &noteSequence, const char *path) {
    fs::FileWriter fileWriter(path);
    if (fileWriter.error() != fs::OK) {
        return fileWriter.error();
    }

    FileHeader header(FileType::NoteSequence, 0, noteSequence.name());
    fileWriter.write(&header, sizeof(header));

    VersionedSerializedWriter writer(
        [&fileWriter] (const void *data, size_t len) { fileWriter.write(data, len); },
        ProjectVersion::Latest
    );

    noteSequence.write(writer);

    return fileWriter.finish();
}

fs::Error FileManager::readNoteSequence(NoteSequence &noteSequence, const char *path) {
    fs::FileReader fileReader(path);
    if (fileReader.error() != fs::OK) {
        return fileReader.error();
    }

    FileHeader header;
    fileReader.read(&header, sizeof(header));

    VersionedSerializedReader reader(
        [&fileReader] (void *data, size_t len) { fileReader.read(data, len); },
        ProjectVersion::Latest
    );

    bool success = noteSequence.read(reader);

    auto error = fileReader.finish();
    if (error == fs::OK && !success) {
        error = fs::INVALID_CHECKSUM;
    }

    return error;
}

fs::Error FileManager::writeCurveSequence(const CurveSequence &curveSequence, const char *path) {
    fs::FileWriter fileWriter(path);
    if (fileWriter.error() != fs::OK) {
        return fileWriter.error();
    }

    FileHeader header(FileType::CurveSequence, 0, curveSequence.name());
    fileWriter.write(&header, sizeof(header));

    VersionedSerializedWriter writer(
        [&fileWriter] (const void *data, size_t len) { fileWriter.write(data, len); },
        ProjectVersion::Latest
    );

    curveSequence.write(writer);

    return fileWriter.finish();
}

fs::Error FileManager::readCurveSequence(CurveSequence &curveSequence, const char *path) {
    fs::FileReader fileReader(path);
    if (fileReader.error() != fs::OK) {
        return fileReader.error();
    }

    FileHeader header;
    fileReader.read(&header, sizeof(header));

    VersionedSerializedReader reader(
        [&fileReader] (void *data, size_t len) { fileReader.read(data, len); },
        ProjectVersion::Latest
    );

    bool success = curveSequence.read(reader);

    auto error = fileReader.finish();
    if (error == fs::OK && !success) {
        error = fs::INVALID_CHECKSUM;
    }

    return error;
}

fs::Error FileManager::writeLogicSequence(const LogicSequence &noteSequence, const char *path) {
    fs::FileWriter fileWriter(path);
    if (fileWriter.error() != fs::OK) {
        return fileWriter.error();
    }

    FileHeader header(FileType::LogicSequence, 0, noteSequence.name());
    fileWriter.write(&header, sizeof(header));

    VersionedSerializedWriter writer(
        [&fileWriter] (const void *data, size_t len) { fileWriter.write(data, len); },
        ProjectVersion::Latest
    );

    noteSequence.write(writer);

    return fileWriter.finish();
}

fs::Error FileManager::readLogicSequence(LogicSequence &noteSequence, const char *path) {
    fs::FileReader fileReader(path);
    if (fileReader.error() != fs::OK) {
        return fileReader.error();
    }

    FileHeader header;
    fileReader.read(&header, sizeof(header));

    VersionedSerializedReader reader(
        [&fileReader] (void *data, size_t len) { fileReader.read(data, len); },
        ProjectVersion::Latest
    );

    bool success = noteSequence.read(reader);

    auto error = fileReader.finish();
    if (error == fs::OK && !success) {
        error = fs::INVALID_CHECKSUM;
    }

    return error;
}

fs::Error FileManager::writeArpSequence(const ArpSequence &noteSequence, const char *path) {
    fs::FileWriter fileWriter(path);
    if (fileWriter.error() != fs::OK) {
        return fileWriter.error();
    }

    FileHeader header(FileType::ArpSequence, 0, noteSequence.name());
    fileWriter.write(&header, sizeof(header));

    VersionedSerializedWriter writer(
        [&fileWriter] (const void *data, size_t len) { fileWriter.write(data, len); },
        ProjectVersion::Latest
    );

    noteSequence.write(writer);

    return fileWriter.finish();
}

fs::Error FileManager::readArpSequence(ArpSequence &noteSequence, const char *path) {
    fs::FileReader fileReader(path);
    if (fileReader.error() != fs::OK) {
        return fileReader.error();
    }

    FileHeader header;
    fileReader.read(&header, sizeof(header));

    VersionedSerializedReader reader(
        [&fileReader] (void *data, size_t len) { fileReader.read(data, len); },
        ProjectVersion::Latest
    );

    bool success = noteSequence.read(reader);

    auto error = fileReader.finish();
    if (error == fs::OK && !success) {
        error = fs::INVALID_CHECKSUM;
    }

    return error;
}

fs::Error FileManager::writeSettings(const Settings &settings, const char *path) {
    fs::FileWriter fileWriter(path);
    if (fileWriter.error() != fs::OK) {
        return fileWriter.error();
    }

    FileHeader header(FileType::Settings, 0, "SETTINGS");
    fileWriter.write(&header, sizeof(header));

    VersionedSerializedWriter writer(
        [&fileWriter] (const void *data, size_t len) { fileWriter.write(data, len); },
        Settings::Version
    );

    settings.write(writer);

    return fileWriter.finish();
}

fs::Error FileManager::readSettings(Settings &settings, const char *path) {
    fs::FileReader fileReader(path);
    if (fileReader.error() != fs::OK) {
        return fileReader.error();
    }

    FileHeader header;
    fileReader.read(&header, sizeof(header));

    VersionedSerializedReader reader(
        [&fileReader] (void *data, size_t len) { fileReader.read(data, len); },
        Settings::Version
    );

    bool success = settings.read(reader);

    auto error = fileReader.finish();
    if (error == fs::OK && !success) {
        error = fs::INVALID_CHECKSUM;
    }

    return error;
}

void FileManager::slotInfo(FileType type, int slot, SlotInfo &info) {
    if (cachedSlot(type, slot, info)) {
        return;
    }

    if (busy()) {
        info.used = false;
        info.name[0] = '\0';
        return;
    }

    info.used = false;

    FixedStringBuilder<32> path;
    slotPath(path, type, slot);

    if (fs::exists(path)) {
        fs::File file(path, fs::File::Read);
        FileHeader header;
        size_t lenRead;
        if (file.read(&header, sizeof(header), &lenRead) == fs::OK && lenRead == sizeof(header)) {
            header.readName(info.name, sizeof(info.name));
            info.used = true;
        }
    }

    cacheSlot(type, slot, info);
}

bool FileManager::slotUsed(FileType type, int slot) {
    if (busy()) {
        return false;
    }

    SlotInfo info;
    slotInfo(type, slot, info);
    return info.used;
}

void FileManager::task(TaskExecuteCallback executeCallback, TaskResultCallback resultCallback) {
    bool reject = false;
    {
        os::LockGuard lock(fileTaskMutex);
        if (_taskPending || _taskExecuting || _taskResultPending) {
            // Reject overlapping tasks deterministically instead of replacing callbacks.
            reject = true;
        } else {
            _taskExecuteCallback = executeCallback;
            _taskResultCallback = resultCallback;
            _taskPending = 1;
        }
    }

    if (reject && resultCallback) {
        resultCallback(fs::INVALID_PARAMETER);
    }
}

void FileManager::processTask() {
    // check volume availability & mount
    uint32_t ticks = os::ticks();
    if (ticks >= _nextVolumeStateCheckTicks) {
        _nextVolumeStateCheckTicks = ticks + os::time::ms(1000);

        uint32_t newVolumeState = fs::volume().available() ? Available : 0;
        if (newVolumeState & Available) {
            if (!(_volumeState & Mounted)) {
                newVolumeState |= (fs::volume().mount() == fs::OK) ? Mounted : 0;
            } else {
                newVolumeState |= Mounted;
            }
        } else {
            invalidateAllSlots();
        }

        _volumeState = newVolumeState;
    }

    TaskExecuteCallback executeCallback;
    TaskResultCallback resultCallback;

    {
        os::LockGuard lock(fileTaskMutex);
        if (_taskPending) {
            executeCallback = _taskExecuteCallback;
            resultCallback = _taskResultCallback;
            _taskExecuteCallback = nullptr;
            _taskResultCallback = nullptr;
            _taskPending = 0;
            _taskExecuting = 1;
        }
    }

    if (executeCallback) {
        fs::Error result = executeCallback();
        os::LockGuard lock(fileTaskMutex);
        _taskExecuting = 0;
        _taskResultPendingValue = result;
        _taskResultCallbackPending = resultCallback;
        _taskResultPending = 1;
    }
}

void FileManager::processTaskResult() {
    TaskResultCallback resultCallback;
    fs::Error result = fs::OK;

    {
        os::LockGuard lock(fileTaskMutex);
        if (!_taskResultPending) {
            return;
        }
        resultCallback = _taskResultCallbackPending;
        result = _taskResultPendingValue;
        _taskResultCallbackPending = nullptr;
        _taskResultPending = 0;
    }

    if (resultCallback) {
        resultCallback(result);
    }
}

bool FileManager::busy() {
    os::LockGuard lock(fileTaskMutex);
    return _taskPending || _taskExecuting || _taskResultPending;
}


fs::Error FileManager::writeFile(FileType type, int slot, std::function<fs::Error(const char *)> write) {
    const auto &info = fileTypeInfos[int(type)];
    if (!fs::exists(info.dir)) {
        fs::mkdir(info.dir);
    }

    FixedStringBuilder<32> path;
    slotPath(path, type, slot);

    auto result = write(path);
    if (result == fs::OK) {
        invalidateSlot(type, slot);
    }

    return result;
}

fs::Error FileManager::readFile(FileType type, int slot, std::function<fs::Error(const char *)> read) {
    const auto &info = fileTypeInfos[int(type)];
    if (!fs::exists(info.dir)) {
        fs::mkdir(info.dir);
    }

    FixedStringBuilder<32> path;
    slotPath(path, type, slot);

    auto result = read(path);

    return result;
}

fs::Error FileManager::writeLastProject(int slot) {
    fs::FileWriter fileWriter("LAST.DAT");
    if (fileWriter.error() != fs::OK) {
        return fileWriter.error();
    }

    fileWriter.write(&slot, sizeof(slot));

    return fileWriter.finish();
}

fs::Error FileManager::readLastProject(int &slot) {
    fs::FileReader fileReader("LAST.DAT");
    if (fileReader.error() != fs::OK) {
        return fileReader.error();
    }

    fileReader.read(&slot, sizeof(slot));

    return fileReader.finish();
}

bool FileManager::cachedSlot(FileType type, int slot, SlotInfo &info) {
    for (auto &cachedSlotInfo : _cachedSlotInfos) {
        if (cachedSlotInfo.ticket != 0 && cachedSlotInfo.type == type && cachedSlotInfo.slot == slot) {
            info = cachedSlotInfo.info;
            cachedSlotInfo.ticket = nextCachedSlotTicket();
            return true;
        }
    }
    return false;
}

void FileManager::cacheSlot(FileType type, int slot, const SlotInfo &info) {
    auto cachedSlotInfo = std::min_element(_cachedSlotInfos.begin(), _cachedSlotInfos.end());
    cachedSlotInfo->type = type;
    cachedSlotInfo->slot = slot;
    cachedSlotInfo->info = info;
    cachedSlotInfo->ticket = nextCachedSlotTicket();
}

void FileManager::invalidateSlot(FileType type, int slot) {
    for (auto &cachedSlotInfo : _cachedSlotInfos) {
        if (cachedSlotInfo.ticket != 0 && cachedSlotInfo.type == type && cachedSlotInfo.slot == slot) {
            cachedSlotInfo.ticket = 0;
        }
    }
}

void FileManager::invalidateAllSlots() {
    for (auto &cachedSlotInfo : _cachedSlotInfos) {
        cachedSlotInfo.ticket = 0;
    }
}

uint32_t FileManager::nextCachedSlotTicket() {
    _cachedSlotInfoTicket = std::max(uint32_t(1), _cachedSlotInfoTicket + 1);
    return _cachedSlotInfoTicket;
}
