#pragma once

#include "FileDefs.h"
#include "Project.h"
#include "Routing.h"
#include "UserScale.h"
#include "Settings.h"

#include "core/fs/FileSystem.h"

#include <array>
#include <functional>

#include <cstdint>

class FileManager {
public:
    static void init();

    static bool volumeAvailable();
    static bool volumeMounted();

    static fs::Error format();

    static fs::Error writeProject(Project &project, int slot);
    static fs::Error readProject(Project &project, int slot);
    static fs::Error readLastProject(Project &project);

    static fs::Error writeUserScale(const UserScale &userScale, int slot);
    static fs::Error readUserScale(UserScale &userScale, int slot);

    static fs::Error writeNoteSequence(const NoteSequence &noteSequence, int slot);
    static fs::Error readNoteSequence(NoteSequence &noteSequence, int slot);
    static fs::Error writeCurveSequence(const CurveSequence &curveSequence, int slot);
    static fs::Error readCurveSequence(CurveSequence &curveSequence, int slot);
    static fs::Error writeLogicSequence(const LogicSequence &logicSequence, int slot);
    static fs::Error readLogicSequence(LogicSequence &logicSequence, int slot);
    static fs::Error writeArpSequence(const ArpSequence &arpSequence, int slot);
    static fs::Error readArpSequence(ArpSequence &arpSequence, int slot);

    static fs::Error writeProject(const Project &project, const char *path);
    static fs::Error readProject(Project &project, const char *path);

    static fs::Error writeUserScale(const UserScale &userScale, const char *path);
    static fs::Error readUserScale(UserScale &userScale, const char *path);

    static fs::Error writeNoteSequence(const NoteSequence &noteSequence, const char *path);
    static fs::Error readNoteSequence(NoteSequence &noteSequence, const char *path);
    static fs::Error writeCurveSequence(const CurveSequence &curveSequence, const char *path);
    static fs::Error readCurveSequence(CurveSequence &curveSequence, const char *path);
    static fs::Error writeLogicSequence(const LogicSequence &logicSequence, const char *path);
    static fs::Error readLogicSequence(LogicSequence &logicSequence, const char *path);
    static fs::Error writeArpSequence(const ArpSequence &arpSequence, const char *path);
    static fs::Error readArpSequence(ArpSequence &arpSequence, const char *path);

    static fs::Error writeSettings(const Settings &settings, const char *path);
    static fs::Error readSettings(Settings &settings, const char *path);

    // Slot information

    struct SlotInfo {
        bool used;
        char name[FileHeader::NameLength + 1];
    };

    static void slotInfo(FileType type, int slot, SlotInfo &info);
    static bool slotUsed(FileType type, int slot);
    static bool busy();

    // File tasks

    typedef std::function<fs::Error(void)> TaskExecuteCallback;
    typedef std::function<void(fs::Error)> TaskResultCallback;

    static void task(TaskExecuteCallback executeCallback, TaskResultCallback resultCallback);
    static void processTask();
    static void processTaskResult();

private:
    static fs::Error writeFile(FileType type, int slot, std::function<fs::Error(const char *)> write);
    static fs::Error readFile(FileType type, int slot, std::function<fs::Error(const char *)> read);

    static fs::Error writeLastProject(int slot);
    static fs::Error readLastProject(int &slot);

    static bool cachedSlot(FileType type, int slot, SlotInfo &info);
    static void cacheSlot(FileType type, int slot, const SlotInfo &info);
    static void invalidateSlot(FileType type, int slot);
    static void invalidateAllSlots();
    static uint32_t nextCachedSlotTicket();

    struct CachedSlotInfo {
        uint32_t ticket = 0;
        FileType type;
        uint8_t slot;
        SlotInfo info;

        bool operator < (const CachedSlotInfo &other) const {
            return ticket < other.ticket;
        }
    };

    enum VolumeState {
        Available   = (1<<0),
        Mounted     = (1<<1),
    };

    static uint32_t _volumeState;
    static uint32_t _nextVolumeStateCheckTicks;

    static std::array<CachedSlotInfo, 4> _cachedSlotInfos;
    static uint32_t _cachedSlotInfoTicket;

    static TaskExecuteCallback _taskExecuteCallback;
    static TaskResultCallback _taskResultCallback;
    static TaskResultCallback _taskResultCallbackPending;
    static fs::Error _taskResultPendingValue;
    static volatile uint32_t _taskPending;
    static volatile uint32_t _taskExecuting;
    static volatile uint32_t _taskResultPending;
};
