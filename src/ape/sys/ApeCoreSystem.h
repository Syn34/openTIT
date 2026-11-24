// openTIT - Ape engine core system start-up and shutdown (memory stack,
// worker threads, file system, pads, root chunk handlers, tags).
#pragma once

namespace ape {

// Sets up the file request scheduler (FIOS on the console).
bool FileSchedulerInit();
bool CoreSystemInit();
bool CoreSystemShutdown();
bool IsCoreSystemRunning();

} // namespace ape
