// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#include "ui/mainwindow.hpp"

int
main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    application.setApplicationName(QLatin1String("bhfconverter"));
    application.setApplicationDisplayName(QLatin1String("Borland Help File Converter"));
    application.setApplicationVersion(QLatin1String("1.0.0"));

    GUI::UI::MainWindow main_window;

    main_window.show();

    return application.exec();
}
