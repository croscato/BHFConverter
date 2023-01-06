// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#include "bhf/file.hpp"
#include "model/context.hpp"
#include "model/index.hpp"
#include "mainwindow.hpp"

namespace GUI {
namespace UI {

struct MainWindowData {
    BHF::File help_file;

    QLabel *stamp = nullptr;
    QLabel *signature = nullptr;
    QLabel *version = nullptr;
    QLabel *file_header = nullptr;
    QLabel *compression = nullptr;

    QTabWidget *tab = nullptr;
    QTableView *tab_context = nullptr;
    QTableView *tab_index = nullptr;
    QLineEdit *edit_context = nullptr;
    QLineEdit *edit_index = nullptr;
    QTextBrowser *text = nullptr;

    Model::Context *model_context = nullptr;
    Model::ContextFilter *proxy_context = nullptr;
    Model::Index *model_index = nullptr;
    Model::IndexFilter *proxy_index = nullptr;
};

static QString
versionFormatToStr(BHF::Version::Format format)
{
    switch (format) {
        case BHF::Version::Invalid : return QObject::tr("Invalid");
        case BHF::Version::TP2     : return QObject::tr("TP2");
        case BHF::Version::TP4     : return QObject::tr("TP4");
        case BHF::Version::TP6     : return QObject::tr("TP6");
        case BHF::Version::BP7     : return QObject::tr("BP7");
    }

    return QObject::tr("Unknown");
}

static QString
compressionTypeToStr(BHF::Compression::Type type)
{
    switch (type) {
        case BHF::Compression::Invalid : return QObject::tr("Invalid");
        case BHF::Compression::Nibble  : return QObject::tr("Nibble");
    }

    return QObject::tr("Unknown");
}

MainWindow::MainWindow(QWidget *parent) noexcept
    : QMainWindow(parent)
    , d{std::make_unique<MainWindowData>()}
{
    d->model_context = new Model::Context(this);
    d->model_index = new Model::Index(this);

    d->proxy_context = new Model::ContextFilter(this);
    d->proxy_context->setSourceModel(d->model_context);

    d->proxy_index = new Model::IndexFilter(this);
    d->proxy_index->setSourceModel(d->model_index);

    setupMenus();
    setupUI();

    d->help_file.open("data/tchelp.tch");

    refreshBHFInformation();
}

MainWindow::~MainWindow() noexcept = default;

void
MainWindow::fileOpen() noexcept
{}

void
MainWindow::fileQuit() noexcept
{
    qApp->quit();
}

void
MainWindow::activatedContext(const QModelIndex &index) noexcept
{
    QModelIndex source_index = d->proxy_context->mapToSource(index);
    QModelIndex context_index = source_index.siblingAtColumn(1);

    int context = d->model_context->data(context_index, Qt::DisplayRole).toInt();

    openContext(context);
}

void
MainWindow::activatedIndex(const QModelIndex &index) noexcept
{
    QModelIndex source_index = d->proxy_index->mapToSource(index);
    QModelIndex key_index = source_index.siblingAtColumn(1);

    int key = d->model_index->data(key_index, Qt::DisplayRole).toInt();

    QModelIndex context_index = d->model_context->index(key, 1);

    int context = d->model_context->data(context_index, Qt::DisplayRole).toInt();

    openContext(context);
}

void
MainWindow::refreshBHFInformation() noexcept
{
    d->stamp->setText(tr("<b>Stamp</b>: %1").arg(d->help_file.stamp().c_str()));

    QByteArray signature = QByteArray::fromStdString(d->help_file.signature());
    signature.chop(1);

    d->signature->setText(tr("<b>Signature</b>: %1").arg(signature.toHex(':')));

    auto version = d->help_file.version();

    d->version->setText(tr("<b>Version</b>: %1 (%2)")
        .arg(versionFormatToStr(version.format))
        .arg(version.text)
    );

    auto file_header = d->help_file.fileHeader();

    d->file_header->setText(tr("<b>File header</b>: options: %1, main index: %2, largest record: %3, size: %4x%5, left margin: %6")
       .arg(file_header.options)
       .arg(file_header.main_index)
       .arg(file_header.largest_record)
       .arg(file_header.width)
       .arg(file_header.height)
       .arg(file_header.left_margin)
    );

    auto compression = d->help_file.compression();
    QByteArray table(reinterpret_cast<const char *>(compression.table), 14);

    d->compression->setText(tr("<b>Compression</b>: %1 [%2]")
        .arg(compressionTypeToStr(compression.type))
        .arg(table.toHex(':'))
    );

    d->model_context->update(d->help_file.context());
    d->model_index->update(d->help_file.index());

    d->tab_context->resizeColumnsToContents();
    d->tab_index->resizeColumnsToContents();
}

void
MainWindow::openContext(int context) noexcept
{
    std::string text = d->help_file.text(context);

    d->text->setText(QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())));
}

void
MainWindow::setupMenus() noexcept
{
    QMenuBar *menu_bar = this->menuBar();

    // File
    QMenu *file = new QMenu(tr("&File"), this);

    QAction *file_open = file->addAction(tr("&Open..."));
    file_open->setShortcut(tr("Ctrl+O"));

    file->addSeparator();

    QAction *file_quit = file->addAction(tr("&Quit"));
    file_quit->setShortcut(tr("Alt+X"));

    menu_bar->addMenu(file);

    connect(file_open, &QAction::triggered, this, &MainWindow::fileOpen);
    connect(file_quit, &QAction::triggered, this, &MainWindow::fileQuit);
}

void
MainWindow::setupUI() noexcept
{
    static constexpr QMargins kLayoutMargin {5, 5, 5, 5};
    static constexpr int kLayoutSpacing = 5;

    QFont font_fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // Line #1
    QHBoxLayout *row_1_layout = new QHBoxLayout;
    row_1_layout->setContentsMargins(0, 0, 0, 0);
    row_1_layout->setSpacing(kLayoutSpacing);

    d->stamp = new QLabel;
    d->signature = new QLabel;
    d->version = new QLabel;

    row_1_layout->addWidget(d->stamp);
    row_1_layout->addWidget(d->signature);
    row_1_layout->addWidget(d->version);

    // Line #2
    QHBoxLayout *row_2_layout = new QHBoxLayout;
    row_2_layout->setContentsMargins(0, 0, 0, 0);
    row_2_layout->setSpacing(kLayoutSpacing);

    d->file_header = new QLabel;
    d->compression = new QLabel;

    row_2_layout->addWidget(d->file_header);
    row_2_layout->addWidget(d->compression);

    // Tab
    QHBoxLayout *main_layout = new QHBoxLayout;
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(kLayoutSpacing);

    d->tab = new QTabWidget;
    d->tab->setMinimumWidth(325);
    d->tab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    d->tab_context = new QTableView;
    d->tab_index = new QTableView;

    connect(d->tab_context, &QTableView::activated, this, &MainWindow::activatedContext);
    connect(d->tab_index, &QTableView::activated, this, &MainWindow::activatedIndex);

    auto sort_context = [this](int index, Qt::SortOrder order) -> void {
        Q_UNUSED(order);

        d->proxy_context->setFilterKeyColumn(index);
    };

    auto sort_index = [this](int index, Qt::SortOrder order) -> void {
        Q_UNUSED(order);

        d->proxy_index->setFilterKeyColumn(index);
    };

    connect(d->tab_context->horizontalHeader(), &QHeaderView::sortIndicatorChanged, sort_context);
    connect(d->tab_index->horizontalHeader(), &QHeaderView::sortIndicatorChanged, sort_index);

    d->edit_context = new QLineEdit;
    d->edit_index = new QLineEdit;

    auto search_context = [this](const QString &search)->void {
        d->proxy_context->setFilterRegularExpression(QRegularExpression::fromWildcard(search + "*"));
    };

    auto search_index = [this](const QString &search)->void {
       d->proxy_index->setFilterRegularExpression(QRegularExpression::fromWildcard(search + "*"));
    };

    connect(d->edit_context, &QLineEdit::textChanged, search_context);
    connect(d->edit_index, &QLineEdit::textChanged, search_index);

    d->tab_context->setModel(d->proxy_context);
    d->tab_context->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->tab_context->setSortingEnabled(true);
    d->tab_context->sortByColumn(0, Qt::AscendingOrder);

    d->tab_index->setModel(d->proxy_index);
    d->tab_index->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->tab_index->setSortingEnabled(true);
    d->tab_index->sortByColumn(0, Qt::AscendingOrder);

    QVBoxLayout *layout_context = new QVBoxLayout;
    layout_context->addWidget(d->edit_context);
    layout_context->addWidget(d->tab_context);
    layout_context->setContentsMargins(kLayoutMargin);
    layout_context->setSpacing(kLayoutSpacing);

    QWidget *widget_context = new QWidget;
    widget_context->setLayout(layout_context);

    QVBoxLayout *layout_index = new QVBoxLayout;
    layout_index->addWidget(d->edit_index);
    layout_index->addWidget(d->tab_index);
    layout_index->setContentsMargins(kLayoutMargin);
    layout_index->setSpacing(kLayoutSpacing);

    QWidget *widget_index = new QWidget;
    widget_index->setLayout(layout_index);

    d->tab->addTab(widget_index, tr("&Index"));
    d->tab->addTab(widget_context, tr("&Context"));

    d->tab_context->verticalHeader()->setVisible(false);
    d->tab_index->verticalHeader()->setVisible(false);

    d->text = new QTextBrowser;
    d->text->setFont(font_fixed);
    d->text->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    main_layout->addWidget(d->tab);
    main_layout->addWidget(d->text);

    // Main layout
    QVBoxLayout *layout = new QVBoxLayout;

    layout->addLayout(row_1_layout);
    layout->addLayout(row_2_layout);
    layout->addLayout(main_layout);

    // Central
    QWidget *central = new QWidget;

    central->setLayout(layout);

    this->setCentralWidget(central);

    d->edit_index->setFocus();
}

} // namespace UI
} // namespace GUI
