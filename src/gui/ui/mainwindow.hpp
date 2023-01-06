// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#ifndef BHFCONVERTER_SRC_GUI_UI_MAINWINDOW_HPP
#define BHFCONVERTER_SRC_GUI_UI_MAINWINDOW_HPP 1

class QModelIndex;

namespace GUI {
namespace UI {

struct MainWindowData;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) noexcept;
    ~MainWindow() noexcept;

private slots:
    void fileOpen() noexcept;
    void fileQuit() noexcept;

    void activatedContext(const QModelIndex &index) noexcept;
    void activatedIndex(const QModelIndex &index) noexcept;

private:
    void refreshBHFInformation() noexcept;
    void openContext(int context) noexcept;
    void setupMenus() noexcept;
    void setupUI() noexcept;

    std::unique_ptr<MainWindowData> d;
};

} // namespace UI
} // namespace GUI

#endif // BHFCONVERTER_SRC_GUI_UI_MAINWINDOW_HPP
