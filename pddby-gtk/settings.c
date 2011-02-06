#include "settings.h"
#include "config.h"

#ifdef _DARWIN_PLATFORM_
#include <Carbon/Carbon.h>
#include <limits.h>
#endif

const gchar *get_share_dir()
{
#ifdef _DARWIN_PLATFORM_
    static gchar share_dir[PATH_MAX] = {0,};
    if (!share_dir[0])
    {
        CFURLRef resourcesURL;
        resourcesURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
        CFURLGetFileSystemRepresentation(resourcesURL, 1, (unsigned char *)share_dir, PATH_MAX);
    }
    return share_dir;
#else
    return PDDBY_SHARE_DIR;
#endif
}
