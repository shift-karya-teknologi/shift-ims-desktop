#ifndef GLOBAL_H
#define GLOBAL_H

#define SIMS_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))

#define SIMS_VERSION_MAJOR  1
#define SIMS_VERSION_MINOR  0
#define SIMS_VERSION_PATCH  0
#define SIMS_VERSION        SIMS_VERSION_CHECK(SIMS_VERSION_MAJOR, SIMS_VERSION_MINOR, SIMS_VERSION_PATCH)
#define SIMS_VERSION_STR    "1.0.0"

#define SIMS_APP_NAME         "Shift IMS"
#define SIMS_APP_DISPLAY_NAME "Shift IMS"

#define SIMS_DEFAULT_SETTINGS_PATH "shift-ims.ini"

#endif // GLOBAL_H
