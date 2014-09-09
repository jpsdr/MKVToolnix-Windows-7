#include "common/common_pch.h"

#if defined(SYS_WINDOWS) && defined(HAVE_QT)

# include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(AccessibleFactory);

#endif
