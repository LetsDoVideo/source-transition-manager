#include "SourceTransitionDock.hpp"
#include <obs-module.h>
#include <plugin-support.h>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QStyle>
#include <QApplication>
#include <QMainWindow>
#include <QDockWidget>
#include <QMessageBox>
#include <obs-properties.h>

static const QList<QPair<QString, QString>> TRANSITION_TYPES = {
    {"cut_transition",           "Cut"},
    {"fade_transition",          "Fade"},
    {"swipe_transition",         "Swipe"},
    {"slide_transition",         "Slide"},
    {"obs_stinger_transition",   "Stinger"},
    {"fade_to_color_transition", "Fade to Color"},
    {"wipe_transition",          "Luma Wipe"},
};

static void onItemSelectSignal(void *data, calldata_t *)
{
    auto *dock = static_cast<SourceTransitionDock *>(data);
    QMetaObject::invokeMethod(dock, "refreshSelectedSources", Qt::QueuedConnection);
}

SourceTransitionDock::SourceTransitionDock(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    obs_frontend_add_event_callback(frontendEventCallback, this);
}

SourceTransitionDock::~SourceTransitionDock()
{
    obs_frontend_remove_event_callback(frontendEventCallback, this);
}

void SourceTransitionDock::cleanup()
{
    disconnectSceneSignals();
    for (auto *item : selectedItems)
        obs_sceneitem_release(item);
    selectedItems.clear();
}

QPushButton *SourceTransitionDock::makeIconButton(QStyle::StandardPixmap icon,
                                                   const QString &tooltip)
{
    auto *btn = new QPushButton(this);
    btn->setIcon(QApplication::style()->standardIcon(icon));
    btn->setFixedSize(22, 22);
    btn->setFlat(true);
    btn->setToolTip(tooltip);
    return btn;
}

void SourceTransitionDock::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Placeholder
    placeholderLabel = new QLabel("Select a source to see transitions", this);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setWordWrap(true);
    placeholderLabel->setStyleSheet("color: gray; padding: 12px;");
    mainLayout->addWidget(placeholderLabel);

    controlsWidget = new QWidget(this);
    controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(6);

    // ── Show Transition Group ──────────────────────────────────────
    QGroupBox   *showGroup  = new QGroupBox(controlsWidget);
    QVBoxLayout *showLayout = new QVBoxLayout(showGroup);
    showLayout->setSpacing(4);
    showGroup->setStyleSheet("QGroupBox { margin-top: 0px; padding-top: 4px; }");

    // Header row: label + cog + copy + paste
    QHBoxLayout *showHeader = new QHBoxLayout();
    showHeader->addWidget(new QLabel("<b>Show Transition</b>", showGroup));
    showHeader->addStretch();

    auto *showCog   = makeIconButton(QStyle::SP_FileDialogDetailedView, "Properties");
    auto *showCopy  = makeIconButton(QStyle::SP_DialogSaveButton,       "Copy");
    auto *showPaste = makeIconButton(QStyle::SP_DialogOpenButton,        "Paste");
    showHeader->addWidget(showCog);
    showHeader->addWidget(showCopy);
    showHeader->addWidget(showPaste);
    showLayout->addLayout(showHeader);

    // Type row
    showTransition = new QComboBox(showGroup);
    for (auto &t : TRANSITION_TYPES)
        showTransition->addItem(t.second, t.first);
    QHBoxLayout *showTypeRow = new QHBoxLayout();
    showTypeRow->addWidget(new QLabel("Type:", showGroup));
    showTypeRow->addWidget(showTransition, 1);
    showLayout->addLayout(showTypeRow);

    // Duration row
    showDuration = new QSpinBox(showGroup);
    showDuration->setRange(0, 10000);
    showDuration->setSingleStep(100);
    showDuration->setSuffix(" ms");
    showDuration->setValue(300);
    QHBoxLayout *showDurRow = new QHBoxLayout();
    showDurRow->addWidget(new QLabel("Duration:", showGroup));
    showDurRow->addWidget(showDuration, 1);
    showLayout->addLayout(showDurRow);

    controlsLayout->addWidget(showGroup);

    // ── Hide Transition Group ──────────────────────────────────────
    QGroupBox   *hideGroup  = new QGroupBox(controlsWidget);
    QVBoxLayout *hideLayout = new QVBoxLayout(hideGroup);
    hideLayout->setSpacing(4);
    hideGroup->setStyleSheet("QGroupBox { margin-top: 0px; padding-top: 4px; }");

    QHBoxLayout *hideHeader = new QHBoxLayout();
    hideHeader->addWidget(new QLabel("<b>Hide Transition</b>", hideGroup));
    hideHeader->addStretch();

    auto *hideCog   = makeIconButton(QStyle::SP_FileDialogDetailedView, "Properties");
    auto *hideCopy  = makeIconButton(QStyle::SP_DialogSaveButton,       "Copy");
    auto *hidePaste = makeIconButton(QStyle::SP_DialogOpenButton,        "Paste");
    hideHeader->addWidget(hideCog);
    hideHeader->addWidget(hideCopy);
    hideHeader->addWidget(hidePaste);
    hideLayout->addLayout(hideHeader);

    hideTransition = new QComboBox(hideGroup);
    for (auto &t : TRANSITION_TYPES)
        hideTransition->addItem(t.second, t.first);
    QHBoxLayout *hideTypeRow = new QHBoxLayout();
    hideTypeRow->addWidget(new QLabel("Type:", hideGroup));
    hideTypeRow->addWidget(hideTransition, 1);
    hideLayout->addLayout(hideTypeRow);

    hideDuration = new QSpinBox(hideGroup);
    hideDuration->setRange(0, 10000);
    hideDuration->setSingleStep(100);
    hideDuration->setSuffix(" ms");
    hideDuration->setValue(300);
    QHBoxLayout *hideDurRow = new QHBoxLayout();
    hideDurRow->addWidget(new QLabel("Duration:", hideGroup));
    hideDurRow->addWidget(hideDuration, 1);
    hideLayout->addLayout(hideDurRow);

    controlsLayout->addWidget(hideGroup);

    // ── Apply to Scene button ──────────────────────────────────────
    applySceneButton = new QPushButton("Apply to All Sources in Scene", controlsWidget);
    controlsLayout->addWidget(applySceneButton);
    controlsLayout->addStretch();

    mainLayout->addWidget(controlsWidget);
    controlsWidget->hide();

    // ── Connections ───────────────────────────────────────────────
    connect(showTransition, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SourceTransitionDock::onShowTransitionChanged);
    connect(hideTransition, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SourceTransitionDock::onHideTransitionChanged);
    connect(showDuration, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SourceTransitionDock::onShowDurationChanged);
    connect(hideDuration, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SourceTransitionDock::onHideDurationChanged);
    connect(showCog,   &QPushButton::clicked, this, &SourceTransitionDock::onShowProperties);
    connect(hideCog,   &QPushButton::clicked, this, &SourceTransitionDock::onHideProperties);
    connect(showCopy,  &QPushButton::clicked, this, &SourceTransitionDock::onCopyShow);
    connect(hideCopy,  &QPushButton::clicked, this, &SourceTransitionDock::onCopyHide);
    connect(showPaste, &QPushButton::clicked, this, &SourceTransitionDock::onPasteShow);
    connect(hidePaste, &QPushButton::clicked, this, &SourceTransitionDock::onPasteHide);
    connect(applySceneButton, &QPushButton::clicked,
            this, &SourceTransitionDock::onApplyToScene);

    adjustSize();
}

void SourceTransitionDock::injectIntoSourcesDock()
{
    auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!mainWindow) return;

    for (auto *dock : mainWindow->findChildren<QDockWidget *>()) {
        if (dock->objectName() == "sourcesDock") {
            auto *titleWidget = new QWidget();
            auto *titleLayout = new QHBoxLayout(titleWidget);
            titleLayout->setContentsMargins(4, 0, 4, 0);
            titleLayout->setSpacing(0);

            auto *stmButton = new QPushButton();
            stmButton->setText("STM");
            stmButton->setFixedSize(32, 18);
            stmButton->setFlat(true);
            stmButton->setToolTip("Source Transition Manager");
            stmButton->setStyleSheet("font-size: 8px; font-weight: bold; padding: 0px;");

            auto *titleLabel = new QLabel(dock->windowTitle());
            titleLabel->setAlignment(Qt::AlignCenter);

            titleLayout->addWidget(stmButton);
            titleLayout->addWidget(titleLabel, 1);

            dock->setTitleBarWidget(titleWidget);

            connect(stmButton, &QPushButton::clicked, this, [this]() {
                auto *parentDock = qobject_cast<QDockWidget *>(
                    this->parentWidget());
                if (parentDock)
                    parentDock->setVisible(!parentDock->isVisible());
            });
            break;
        }
    }
}

void SourceTransitionDock::connectSceneSignals(obs_source_t *scene_source)
{
    if (!scene_source) return;
    signal_handler_t *sh = obs_source_get_signal_handler(scene_source);
    if (!sh) return;
    signal_handler_connect(sh, "item_select",   onItemSelectSignal, this);
    signal_handler_connect(sh, "item_deselect", onItemSelectSignal, this);
    currentScene = scene_source;
}

void SourceTransitionDock::disconnectSceneSignals()
{
    if (!currentScene) return;
    signal_handler_t *sh = obs_source_get_signal_handler(currentScene);
    if (sh) {
        signal_handler_disconnect(sh, "item_select",   onItemSelectSignal, this);
        signal_handler_disconnect(sh, "item_deselect", onItemSelectSignal, this);
    }
    currentScene = nullptr;
}

void SourceTransitionDock::frontendEventCallback(obs_frontend_event event, void *data)
{
    auto *dock = static_cast<SourceTransitionDock *>(data);
    switch (event) {
    case OBS_FRONTEND_EVENT_FINISHED_LOADING:
        QMetaObject::invokeMethod(dock, "onSceneChanged", Qt::QueuedConnection);
        dock->injectIntoSourcesDock();
        break;
    case OBS_FRONTEND_EVENT_SCENE_CHANGED:
    case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
        QMetaObject::invokeMethod(dock, "onSceneChanged", Qt::QueuedConnection);
        break;
    case OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN:
    case OBS_FRONTEND_EVENT_EXIT:
        QMetaObject::invokeMethod(dock, "cleanup", Qt::QueuedConnection);
        break;
    default:
        break;
    }
}

void SourceTransitionDock::onSceneChanged()
{
    disconnectSceneSignals();

    obs_source_t *scene_source = obs_frontend_get_current_scene();
    if (!scene_source) {
        placeholderLabel->show();
        controlsWidget->hide();
        return;
    }

    connectSceneSignals(scene_source);
    obs_source_release(scene_source);
    refreshSelectedSources();
}

void SourceTransitionDock::refreshSelectedSources()
{
    for (auto *item : selectedItems)
        obs_sceneitem_release(item);
    selectedItems.clear();

    obs_source_t *scene_source = obs_frontend_get_current_scene();
    if (!scene_source) {
        placeholderLabel->show();
        controlsWidget->hide();
        return;
    }

    obs_scene_t *scene = obs_scene_from_source(scene_source);
    obs_source_release(scene_source);

    obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *data) -> bool {
        if (obs_sceneitem_selected(item)) {
            auto *dock = static_cast<SourceTransitionDock *>(data);
            obs_sceneitem_addref(item);
            dock->selectedItems.append(item);
        }
        return true;
    }, this);

    if (selectedItems.isEmpty()) {
        placeholderLabel->show();
        controlsWidget->hide();
        return;
    }

    placeholderLabel->hide();
    controlsWidget->show();
    loadTransitionsForItem(selectedItems.first());
}

void SourceTransitionDock::loadTransitionsForItem(obs_sceneitem_t *item)
{
    showTransition->blockSignals(true);
    hideTransition->blockSignals(true);
    showDuration->blockSignals(true);
    hideDuration->blockSignals(true);

    obs_source_t *showTr = obs_sceneitem_get_transition(item, true);
    if (showTr) {
        int idx = showTransition->findData(QString(obs_source_get_id(showTr)));
        if (idx >= 0) showTransition->setCurrentIndex(idx);
    } else {
        showTransition->setCurrentIndex(0);
    }
    showDuration->setValue((int)obs_sceneitem_get_transition_duration(item, true));

    obs_source_t *hideTr = obs_sceneitem_get_transition(item, false);
    if (hideTr) {
        int idx = hideTransition->findData(QString(obs_source_get_id(hideTr)));
        if (idx >= 0) hideTransition->setCurrentIndex(idx);
    } else {
        hideTransition->setCurrentIndex(0);
    }
    hideDuration->setValue((int)obs_sceneitem_get_transition_duration(item, false));

    showTransition->blockSignals(false);
    hideTransition->blockSignals(false);
    showDuration->blockSignals(false);
    hideDuration->blockSignals(false);
}

void SourceTransitionDock::applyTransitionToItem(obs_sceneitem_t *item,
    const QString &showId, int showDur,
    const QString &hideId, int hideDur)
{
    obs_source_t *showTr = obs_source_create_private(
        showId.toUtf8().constData(), nullptr, nullptr);
    if (showTr) {
        obs_sceneitem_set_transition(item, true, showTr);
        obs_sceneitem_set_transition_duration(item, true, (uint32_t)showDur);
        obs_source_release(showTr);
    }
    obs_source_t *hideTr = obs_source_create_private(
        hideId.toUtf8().constData(), nullptr, nullptr);
    if (hideTr) {
        obs_sceneitem_set_transition(item, false, hideTr);
        obs_sceneitem_set_transition_duration(item, false, (uint32_t)hideDur);
        obs_source_release(hideTr);
    }
}

void SourceTransitionDock::openProperties(bool show)
{
    if (selectedItems.isEmpty()) return;
    obs_source_t *tr = obs_sceneitem_get_transition(selectedItems.first(), show);
    if (!tr) return;
    obs_frontend_open_source_properties(tr);
}

void SourceTransitionDock::onShowProperties() { openProperties(true);  }
void SourceTransitionDock::onHideProperties() { openProperties(false); }

void SourceTransitionDock::onCopyShow()
{
    if (selectedItems.isEmpty()) return;
    obs_source_t *tr = obs_sceneitem_get_transition(selectedItems.first(), true);
    showClipboard.typeId   = tr ? QString(obs_source_get_id(tr))
                                : showTransition->currentData().toString();
    showClipboard.duration = showDuration->value();
    showClipboard.hasData  = true;
}

void SourceTransitionDock::onCopyHide()
{
    if (selectedItems.isEmpty()) return;
    obs_source_t *tr = obs_sceneitem_get_transition(selectedItems.first(), false);
    hideClipboard.typeId   = tr ? QString(obs_source_get_id(tr))
                                : hideTransition->currentData().toString();
    hideClipboard.duration = hideDuration->value();
    hideClipboard.hasData  = true;
}

void SourceTransitionDock::onPasteShow()
{
    if (selectedItems.isEmpty()) return;

    TransitionClipboard &src = showClipboard.hasData ? showClipboard : hideClipboard;
    if (!src.hasData) return;

    showTransition->blockSignals(true);
    showDuration->blockSignals(true);

    int idx = showTransition->findData(src.typeId);
    if (idx >= 0) showTransition->setCurrentIndex(idx);
    showDuration->setValue(src.duration);

    showTransition->blockSignals(false);
    showDuration->blockSignals(false);

    for (auto *item : selectedItems) {
        obs_source_t *tr = obs_source_create_private(
            src.typeId.toUtf8().constData(), nullptr, nullptr);
        if (tr) {
            obs_sceneitem_set_transition(item, true, tr);
            obs_sceneitem_set_transition_duration(item, true, (uint32_t)src.duration);
            obs_source_release(tr);
        }
    }
}

void SourceTransitionDock::onPasteHide()
{
    if (selectedItems.isEmpty()) return;

    TransitionClipboard &src = hideClipboard.hasData ? hideClipboard : showClipboard;
    if (!src.hasData) return;

    hideTransition->blockSignals(true);
    hideDuration->blockSignals(true);

    int idx = hideTransition->findData(src.typeId);
    if (idx >= 0) hideTransition->setCurrentIndex(idx);
    hideDuration->setValue(src.duration);

    hideTransition->blockSignals(false);
    hideDuration->blockSignals(false);

    for (auto *item : selectedItems) {
        obs_source_t *tr = obs_source_create_private(
            src.typeId.toUtf8().constData(), nullptr, nullptr);
        if (tr) {
            obs_sceneitem_set_transition(item, false, tr);
            obs_sceneitem_set_transition_duration(item, false, (uint32_t)src.duration);
            obs_source_release(tr);
        }
    }
}

void SourceTransitionDock::onShowTransitionChanged()
{
    if (selectedItems.isEmpty()) return;
    QString id  = showTransition->currentData().toString();
    int     dur = showDuration->value();
    for (auto *item : selectedItems) {
        obs_source_t *tr = obs_source_create_private(
            id.toUtf8().constData(), nullptr, nullptr);
        if (tr) {
            obs_sceneitem_set_transition(item, true, tr);
            obs_sceneitem_set_transition_duration(item, true, (uint32_t)dur);
            obs_source_release(tr);
        }
    }
}

void SourceTransitionDock::onHideTransitionChanged()
{
    if (selectedItems.isEmpty()) return;
    QString id  = hideTransition->currentData().toString();
    int     dur = hideDuration->value();
    for (auto *item : selectedItems) {
        obs_source_t *tr = obs_source_create_private(
            id.toUtf8().constData(), nullptr, nullptr);
        if (tr) {
            obs_sceneitem_set_transition(item, false, tr);
            obs_sceneitem_set_transition_duration(item, false, (uint32_t)dur);
            obs_source_release(tr);
        }
    }
}

void SourceTransitionDock::onShowDurationChanged()
{
    if (selectedItems.isEmpty()) return;
    int dur = showDuration->value();
    for (auto *item : selectedItems)
        obs_sceneitem_set_transition_duration(item, true, (uint32_t)dur);
}

void SourceTransitionDock::onHideDurationChanged()
{
    if (selectedItems.isEmpty()) return;
    int dur = hideDuration->value();
    for (auto *item : selectedItems)
        obs_sceneitem_set_transition_duration(item, false, (uint32_t)dur);
}

void SourceTransitionDock::onApplyToScene()
{
    auto reply = QMessageBox::question(this,
        "Apply to All Sources",
        "Apply current Show and Hide transition settings to every source in the current scene?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QString showId  = showTransition->currentData().toString();
    int     showDur = showDuration->value();
    QString hideId  = hideTransition->currentData().toString();
    int     hideDur = hideDuration->value();

    obs_source_t *scene_source = obs_frontend_get_current_scene();
    if (!scene_source) return;

    obs_scene_t *scene = obs_scene_from_source(scene_source);
    obs_source_release(scene_source);

    struct ApplyData {
        SourceTransitionDock *dock;
        QString showId;
        int     showDur;
        QString hideId;
        int     hideDur;
    } applyData = {this, showId, showDur, hideId, hideDur};

    obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *data) -> bool {
        auto *d = static_cast<ApplyData *>(data);
        d->dock->applyTransitionToItem(item, d->showId, d->showDur,
                                              d->hideId, d->hideDur);
        return true;
    }, &applyData);
}
