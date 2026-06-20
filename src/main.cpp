#include <QApplication>
#include <cstring>
#include "mainwindow.h"
#include "testmode.h"

int main(int argc, char* argv[]) {
    bool testMode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--test") == 0) { testMode = true; break; }
    }

    if (testMode) {
        // Force the offscreen platform so the UI smoke tests work without a display
        qputenv("QT_QPA_PLATFORM", "offscreen");
        // Suppress Qt logging noise during tests
        qputenv("QT_LOGGING_RULES", "*.debug=false;qt.qpa.*=false");
    }

    QApplication app(argc, argv);
    app.setApplicationName("BCScan");
    app.setOrganizationName("BCScan");
    app.setApplicationVersion("1.0.0");

    if (testMode)
        return runTests();

    MainWindow window;
    window.show();
    return app.exec();
}
